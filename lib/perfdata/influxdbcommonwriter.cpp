/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "perfdata/influxdbcommonwriter.hpp"
#include "perfdata/influxdbcommonwriter-ti.cpp"
#include "remote/url.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "base/application.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/stream.hpp"
#include "base/json.hpp"
#include "base/networkstream.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include "base/tlsutility.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>
#include <memory>
#include <string>
#include <utility>

using namespace icinga;

REGISTER_TYPE(InfluxdbCommonWriter);

class InfluxdbInteger final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(InfluxdbInteger);

	InfluxdbInteger(int value)
		: m_Value(value)
	{ }

	int GetValue() const
	{
		return m_Value;
	}

private:
	int m_Value;
};

void InfluxdbCommonWriter::OnConfigLoaded()
{
	ObjectImpl<InfluxdbCommonWriter>::OnConfigLoaded();

	m_WorkQueue.SetName(GetReflectionType()->GetName() + ", " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, GetReflectionType()->GetName())
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void InfluxdbCommonWriter::Resume()
{
	ObjectImpl<InfluxdbCommonWriter>::Resume();

	Log(LogInformation, GetReflectionType()->GetName())
		<< "'" << GetName() << "' resumed.";

	/* Register exception handler for WQ tasks. */
	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	/* Setup timer for periodically flushing m_DataBuffer */
	m_FlushTimer = new Timer();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect([this](const Timer * const&) { FlushTimeout(); });
	m_FlushTimer->Start();
	m_FlushTimer->Reschedule(0);

	/* Register for new metrics. */
	m_HandleCheckResults = Checkable::OnNewCheckResult.connect([this](const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
		CheckResultHandler(checkable, cr);
	});
}

/* Pause is equivalent to Stop, but with HA capabilities to resume at runtime. */
void InfluxdbCommonWriter::Pause()
{
	m_HandleCheckResults.disconnect();

	/* Force a flush. */
	Log(LogDebug, GetReflectionType()->GetName())
		<< "Processing pending tasks and flushing data buffers.";

	m_WorkQueue.Enqueue([this]() { FlushWQ(); }, PriorityLow);

	/* Wait for the flush to complete, implicitly waits for all WQ tasks enqueued prior to pausing. */
	m_WorkQueue.Join();

	Log(LogInformation, GetReflectionType()->GetName())
		<< "'" << GetName() << "' paused.";

	ObjectImpl<InfluxdbCommonWriter>::Pause();
}

void InfluxdbCommonWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void InfluxdbCommonWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, GetReflectionType()->GetName(), "Exception during InfluxDB operation: Verify that your backend is operational!");

	Log(LogDebug, GetReflectionType()->GetName())
		<< "Exception during InfluxDB operation: " << DiagnosticInformation(std::move(exp));

	//TODO: Close the connection, if we keep it open.
}

OptionalTlsStream InfluxdbCommonWriter::Connect()
{
	Log(LogNotice, GetReflectionType()->GetName())
		<< "Reconnecting to InfluxDB on host '" << GetHost() << "' port '" << GetPort() << "'.";

	OptionalTlsStream stream;
	bool ssl = GetSslEnable();

	if (ssl) {
		Shared<boost::asio::ssl::context>::Ptr sslContext;

		try {
			sslContext = MakeAsioSslContext(GetSslCert(), GetSslKey(), GetSslCaCert());
		} catch (const std::exception& ex) {
			Log(LogWarning, GetReflectionType()->GetName())
				<< "Unable to create SSL context.";
			throw;
		}

		stream.first = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslContext, GetHost());

	} else {
		stream.second = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());
	}

	try {
		icinga::Connect(ssl ? stream.first->lowest_layer() : stream.second->lowest_layer(), GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogWarning, GetReflectionType()->GetName())
			<< "Can't connect to InfluxDB on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw;
	}

	if (ssl) {
		auto& tlsStream (stream.first->next_layer());

		try {
			tlsStream.handshake(tlsStream.client);
		} catch (const std::exception& ex) {
			Log(LogWarning, GetReflectionType()->GetName())
				<< "TLS handshake with host '" << GetHost() << "' failed.";
			throw;
		}

		if (!GetSslInsecureNoverify()) {
			if (!tlsStream.GetPeerCertificate()) {
				BOOST_THROW_EXCEPTION(std::runtime_error("InfluxDB didn't present any TLS certificate."));
			}

			if (!tlsStream.IsVerifyOK()) {
				BOOST_THROW_EXCEPTION(std::runtime_error(
					"TLS certificate validation failed: " + std::string(tlsStream.GetVerifyError())
				));
			}
		}
	}

	return std::move(stream);
}

void InfluxdbCommonWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	m_WorkQueue.Enqueue([this, checkable, cr]() { CheckResultHandlerWQ(checkable, cr); }, PriorityLow);
}

void InfluxdbCommonWriter::CheckResultHandlerWQ(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	CONTEXT("Processing check result for '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String prefix;

	double ts = cr->GetExecutionEnd();

	// Clone the template and perform an in-place macro expansion of measurement and tag values
	Dictionary::Ptr tmpl_clean = service ? GetServiceTemplate() : GetHostTemplate();
	Dictionary::Ptr tmpl = static_pointer_cast<Dictionary>(tmpl_clean->ShallowClone());
	tmpl->Set("measurement", MacroProcessor::ResolveMacros(tmpl->Get("measurement"), resolvers, cr));

	Dictionary::Ptr tagsClean = tmpl->Get("tags");
	if (tagsClean) {
		Dictionary::Ptr tags = new Dictionary();

		{
			ObjectLock olock(tagsClean);
			for (const Dictionary::Pair& pair : tagsClean) {
				String missing_macro;
				Value value = MacroProcessor::ResolveMacros(pair.second, resolvers, cr, &missing_macro);

				if (missing_macro.IsEmpty()) {
					tags->Set(pair.first, value);
				}
			}
		}

		tmpl->Set("tags", tags);
	}

	CheckCommand::Ptr checkCommand = checkable->GetCheckCommand();

	Array::Ptr perfdata = cr->GetPerformanceData();

	if (perfdata) {
		ObjectLock olock(perfdata);
		for (const Value& val : perfdata) {
			PerfdataValue::Ptr pdv;

			if (val.IsObjectType<PerfdataValue>())
				pdv = val;
			else {
				try {
					pdv = PerfdataValue::Parse(val);
				} catch (const std::exception&) {
					Log(LogWarning, GetReflectionType()->GetName())
						<< "Ignoring invalid perfdata for checkable '"
						<< checkable->GetName() << "' and command '"
						<< checkCommand->GetName() << "' with value: " << val;
					continue;
				}
			}

			Dictionary::Ptr fields = new Dictionary();
			fields->Set("value", pdv->GetValue());

			if (GetEnableSendThresholds()) {
				if (!pdv->GetCrit().IsEmpty())
					fields->Set("crit", pdv->GetCrit());
				if (!pdv->GetWarn().IsEmpty())
					fields->Set("warn", pdv->GetWarn());
				if (!pdv->GetMin().IsEmpty())
					fields->Set("min", pdv->GetMin());
				if (!pdv->GetMax().IsEmpty())
					fields->Set("max", pdv->GetMax());
			}
			if (!pdv->GetUnit().IsEmpty()) {
				fields->Set("unit", pdv->GetUnit());
			}

			SendMetric(checkable, tmpl, pdv->GetLabel(), fields, ts);
		}
	}

	if (GetEnableSendMetadata()) {
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		Dictionary::Ptr fields = new Dictionary();

		if (service)
			fields->Set("state", new InfluxdbInteger(service->GetState()));
		else
			fields->Set("state", new InfluxdbInteger(host->GetState()));

		fields->Set("current_attempt", new InfluxdbInteger(checkable->GetCheckAttempt()));
		fields->Set("max_check_attempts", new InfluxdbInteger(checkable->GetMaxCheckAttempts()));
		fields->Set("state_type", new InfluxdbInteger(checkable->GetStateType()));
		fields->Set("reachable", checkable->IsReachable());
		fields->Set("downtime_depth", new InfluxdbInteger(checkable->GetDowntimeDepth()));
		fields->Set("acknowledgement", new InfluxdbInteger(checkable->GetAcknowledgement()));
		fields->Set("latency", cr->CalculateLatency());
		fields->Set("execution_time", cr->CalculateExecutionTime());

		SendMetric(checkable, tmpl, Empty, fields, ts);
	}
}

String InfluxdbCommonWriter::EscapeKeyOrTagValue(const String& str)
{
	// Iterate over the key name and escape commas and spaces with a backslash
	String result = str;
	boost::algorithm::replace_all(result, "\"", "\\\"");
	boost::algorithm::replace_all(result, "=", "\\=");
	boost::algorithm::replace_all(result, ",", "\\,");
	boost::algorithm::replace_all(result, " ", "\\ ");

	// InfluxDB 'feature': although backslashes are allowed in keys they also act
	// as escape sequences when followed by ',' or ' '.  When your tag is like
	// 'metric=C:\' bad things happen.  Backslashes themselves cannot be escaped
	// and through experimentation they also escape '='.  To be safe we replace
	// trailing backslashes with and underscore.
	// See https://github.com/influxdata/influxdb/issues/8587 for more info
	size_t length = result.GetLength();
	if (result[length - 1] == '\\')
		result[length - 1] = '_';

	return result;
}

String InfluxdbCommonWriter::EscapeValue(const Value& value)
{
	if (value.IsObjectType<InfluxdbInteger>()) {
		std::ostringstream os;
		os << static_cast<InfluxdbInteger::Ptr>(value)->GetValue() << "i";
		return os.str();
	}

	if (value.IsBoolean())
		return value ? "true" : "false";

	if (value.IsString())
		return "\"" + EscapeKeyOrTagValue(value) + "\"";

	return value;
}

void InfluxdbCommonWriter::SendMetric(const Checkable::Ptr& checkable, const Dictionary::Ptr& tmpl,
	const String& label, const Dictionary::Ptr& fields, double ts)
{
	std::ostringstream msgbuf;
	msgbuf << EscapeKeyOrTagValue(tmpl->Get("measurement"));

	Dictionary::Ptr tags = tmpl->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			// Empty macro expansion, no tag
			if (!pair.second.IsEmpty()) {
				msgbuf << "," << EscapeKeyOrTagValue(pair.first) << "=" << EscapeKeyOrTagValue(pair.second);
			}
		}
	}

	// Label may be empty in the case of metadata
	if (!label.IsEmpty())
		msgbuf << ",metric=" << EscapeKeyOrTagValue(label);

	msgbuf << " ";

	{
		bool first = true;

		ObjectLock fieldLock(fields);
		for (const Dictionary::Pair& pair : fields) {
			if (first)
				first = false;
			else
				msgbuf << ",";

			msgbuf << EscapeKeyOrTagValue(pair.first) << "=" << EscapeValue(pair.second);
		}
	}

	msgbuf << " " <<  static_cast<unsigned long>(ts);

	Log(LogDebug, GetReflectionType()->GetName())
		<< "Checkable '" << checkable->GetName() << "' adds to metric list:'" << msgbuf.str() << "'.";

	// Buffer the data point
	m_DataBuffer.emplace_back(msgbuf.str());
	m_DataBufferSize = m_DataBuffer.size();

	// Flush if we've buffered too much to prevent excessive memory use
	if (static_cast<int>(m_DataBuffer.size()) >= GetFlushThreshold()) {
		Log(LogDebug, GetReflectionType()->GetName())
			<< "Data buffer overflow writing " << m_DataBuffer.size() << " data points";

		try {
			FlushWQ();
		} catch (...) {
			/* Do nothing. */
		}
	}
}

void InfluxdbCommonWriter::FlushTimeout()
{
	m_WorkQueue.Enqueue([this]() { FlushTimeoutWQ(); }, PriorityHigh);
}

void InfluxdbCommonWriter::FlushTimeoutWQ()
{
	AssertOnWorkQueue();

	Log(LogDebug, GetReflectionType()->GetName())
		<< "Timer expired writing " << m_DataBuffer.size() << " data points";

	FlushWQ();
}

void InfluxdbCommonWriter::FlushWQ()
{
	AssertOnWorkQueue();

	namespace beast = boost::beast;
	namespace http = beast::http;

	/* Flush can be called from 1) Timeout 2) Threshold 3) on shutdown/reload. */
	if (m_DataBuffer.empty())
		return;

	Log(LogDebug, GetReflectionType()->GetName())
		<< "Flushing data buffer to InfluxDB.";

	String body = boost::algorithm::join(m_DataBuffer, "\n");
	m_DataBuffer.clear();
	m_DataBufferSize = 0;

	OptionalTlsStream stream;

	try {
		stream = Connect();
	} catch (const std::exception& ex) {
		Log(LogWarning, GetReflectionType()->GetName())
			<< "Flush failed, cannot connect to InfluxDB: " << DiagnosticInformation(ex, false);
		return;
	}

	Defer s ([&stream]() {
		if (stream.first) {
			stream.first->next_layer().shutdown();
		}
	});

	auto request (AssembleRequest(std::move(body)));

	try {
		if (stream.first) {
			http::write(*stream.first, request);
			stream.first->flush();
		} else {
			http::write(*stream.second, request);
			stream.second->flush();
		}
	} catch (const std::exception& ex) {
		Log(LogWarning, GetReflectionType()->GetName())
			<< "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw;
	}

	http::parser<false, http::string_body> parser;
	beast::flat_buffer buf;

	try {
		if (stream.first) {
			http::read(*stream.first, buf, parser);
		} else {
			http::read(*stream.second, buf, parser);
		}
	} catch (const std::exception& ex) {
		Log(LogWarning, GetReflectionType()->GetName())
			<< "Failed to parse HTTP response from host '" << GetHost() << "' port '" << GetPort() << "': " << DiagnosticInformation(ex);
		throw;
	}

	auto& response (parser.get());

	if (response.result() != http::status::no_content) {
		Log(LogWarning, GetReflectionType()->GetName())
			<< "Unexpected response code: " << response.result();

		auto& contentType (response[http::field::content_type]);
		if (contentType != "application/json") {
			Log(LogWarning, GetReflectionType()->GetName())
				<< "Unexpected Content-Type: " << contentType;
			return;
		}

		Dictionary::Ptr jsonResponse;
		auto& body (response.body());

		try {
			jsonResponse = JsonDecode(body);
		} catch (...) {
			Log(LogWarning, GetReflectionType()->GetName())
				<< "Unable to parse JSON response:\n" << body;
			return;
		}

		String error = jsonResponse->Get("error");

		Log(LogCritical, GetReflectionType()->GetName())
			<< "InfluxDB error message:\n" << error;
	}
}

boost::beast::http::request<boost::beast::http::string_body> InfluxdbCommonWriter::AssembleBaseRequest(String body)
{
	namespace http = boost::beast::http;

	auto url (AssembleUrl());
	http::request<http::string_body> request (http::verb::post, std::string(url->Format(true)), 10);

	request.set(http::field::user_agent, "Icinga/" + Application::GetAppVersion());
	request.set(http::field::host, url->GetHost() + ":" + url->GetPort());
	request.body() = std::move(body);
	request.content_length(request.body().size());

	return std::move(request);
}

Url::Ptr InfluxdbCommonWriter::AssembleBaseUrl()
{
	Url::Ptr url = new Url();

	url->SetScheme(GetSslEnable() ? "https" : "http");
	url->SetHost(GetHost());
	url->SetPort(GetPort());
	url->AddQueryElement("precision", "s");

	return std::move(url);
}

void InfluxdbCommonWriter::ValidateHostTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<InfluxdbCommonWriter>::ValidateHostTemplate(lvalue, utils);

	String measurement = lvalue()->Get("measurement");
	if (!MacroProcessor::ValidateMacroString(measurement))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "host_template", "measurement" }, "Closing $ not found in macro format string '" + measurement + "'."));

	Dictionary::Ptr tags = lvalue()->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			if (!MacroProcessor::ValidateMacroString(pair.second))
				BOOST_THROW_EXCEPTION(ValidationError(this, { "host_template", "tags", pair.first }, "Closing $ not found in macro format string '" + pair.second));
		}
	}
}

void InfluxdbCommonWriter::ValidateServiceTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<InfluxdbCommonWriter>::ValidateServiceTemplate(lvalue, utils);

	String measurement = lvalue()->Get("measurement");
	if (!MacroProcessor::ValidateMacroString(measurement))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "service_template", "measurement" }, "Closing $ not found in macro format string '" + measurement + "'."));

	Dictionary::Ptr tags = lvalue()->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			if (!MacroProcessor::ValidateMacroString(pair.second))
				BOOST_THROW_EXCEPTION(ValidationError(this, { "service_template", "tags", pair.first }, "Closing $ not found in macro format string '" + pair.second));
		}
	}
}

