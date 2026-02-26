// SPDX-FileCopyrightText: 2021 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "perfdata/influxdbcommonwriter.hpp"
#include "base/defer.hpp"
#include "perfdata/influxdbcommonwriter-ti.cpp"
#include "remote/url.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/json.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>
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
	m_FlushTimer = Timer::Create();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect([this](const Timer * const&) { FlushTimeout(); });
	m_FlushTimer->Start();
	m_FlushTimer->Reschedule(0);

	Shared<boost::asio::ssl::context>::Ptr sslContext;
	if (GetSslEnable()) {
		try {
			sslContext = MakeAsioSslContext(GetSslCert(), GetSslKey(), GetSslCaCert());
		} catch (const std::exception& ex) {
			Log(LogWarning, GetReflectionType()->GetName())
				<< "Unable to create SSL context.";
			throw;
		}
	}
	
	m_Connection = new PerfdataWriterConnection{GetReflectionType()->GetName(), GetName(), GetHost(), GetPort(), sslContext, !GetSslInsecureNoverify()};

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

	m_FlushTimer->Stop(true);

	std::promise<void> queueDonePromise;
	m_WorkQueue.Enqueue([&]() {
		FlushWQ();
		queueDonePromise.set_value();
	}, PriorityLow);

	auto timeout = std::chrono::duration<double>{GetDisconnectTimeout()};
	m_Connection->CancelAfterTimeout(queueDonePromise.get_future(), timeout);

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
}

void InfluxdbCommonWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);

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

	Dictionary::Ptr fields;
	if (GetEnableSendMetadata()) {
		fields = new Dictionary();

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
	}

	m_WorkQueue.Enqueue([this, checkable, cr, tmpl = std::move(tmpl), metadataFields = std::move(fields)]() {
		if (m_Connection->IsStopped()) {
			return;
		}

		CONTEXT("Processing check result for '" << checkable->GetName() << "'");

		double ts = cr->GetExecutionEnd();

		if (Array::Ptr perfdata = cr->GetPerformanceData()) {
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
							<< checkable->GetCheckCommand()->GetName() << "' with value: " << val;
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

		if (metadataFields) {
			SendMetric(checkable, tmpl, Empty, metadataFields, ts);
		}
	}, PriorityLow);
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
	AssertOnWorkQueue();

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

/**
 * Queues a Flush on the work-queue and restarts the timer.
 */
void InfluxdbCommonWriter::FlushTimeout()
{
	if (m_FlushTimerInQueue.exchange(true, std::memory_order_relaxed)) {
		return;
	}

	m_WorkQueue.Enqueue([&]() {
		Defer resetFlushTimer{[&]() { m_FlushTimerInQueue.store(false, std::memory_order_relaxed); }};
		FlushWQ();
	});
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

	auto request (AssembleRequest(std::move(body)));

	decltype(m_Connection->Send(request)) response;
	try {
		response = m_Connection->Send(request);
	} catch (const PerfdataWriterConnection::Stopped& ex) {
		Log(LogDebug, GetReflectionType()->GetName()) << ex.what();
		return;
	}

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

	return request;
}

Url::Ptr InfluxdbCommonWriter::AssembleBaseUrl()
{
	Url::Ptr url = new Url();

	url->SetScheme(GetSslEnable() ? "https" : "http");
	url->SetHost(GetHost());
	url->SetPort(GetPort());
	url->AddQueryElement("precision", "s");

	return url;
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
				BOOST_THROW_EXCEPTION(ValidationError(this, { "host_template", "tags", pair.first }, "Closing $ not found in macro format string '" + pair.second + "'."));
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
				BOOST_THROW_EXCEPTION(ValidationError(this, { "service_template", "tags", pair.first }, "Closing $ not found in macro format string '" + pair.second + "'."));
		}
	}
}
