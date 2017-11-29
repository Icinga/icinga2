/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "perfdata/influxdbwriter.hpp"
#include "perfdata/influxdbwriter.tcpp"
#include "remote/url.hpp"
#include "remote/httprequest.hpp"
#include "remote/httpresponse.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/stream.hpp"
#include "base/json.hpp"
#include "base/networkstream.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include "base/tlsutility.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>

using namespace icinga;

REGISTER_TYPE(InfluxdbWriter);

REGISTER_STATSFUNCTION(InfluxdbWriter, &InfluxdbWriter::StatsFunc);

InfluxdbWriter::InfluxdbWriter(void)
    : m_WorkQueue(10000000, 1)
{ }

void InfluxdbWriter::OnConfigLoaded(void)
{
	ObjectImpl<InfluxdbWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("InfluxdbWriter, " + GetName());
}

void InfluxdbWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const InfluxdbWriter::Ptr& influxdbwriter : ConfigType::GetObjectsByType<InfluxdbWriter>()) {
		size_t workQueueItems = influxdbwriter->m_WorkQueue.GetLength();
		double workQueueItemRate = influxdbwriter->m_WorkQueue.GetTaskCount(60) / 60.0;
		size_t dataBufferItems = influxdbwriter->m_DataBuffer.size();

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("work_queue_items", workQueueItems);
		stats->Set("work_queue_item_rate", workQueueItemRate);
		stats->Set("data_buffer_items", dataBufferItems);

		nodes->Set(influxdbwriter->GetName(), stats);

		perfdata->Add(new PerfdataValue("influxdbwriter_" + influxdbwriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("influxdbwriter_" + influxdbwriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
		perfdata->Add(new PerfdataValue("influxdbwriter_" + influxdbwriter->GetName() + "_data_queue_items", dataBufferItems));
	}

	status->Set("influxdbwriter", nodes);
}

void InfluxdbWriter::Start(bool runtimeCreated)
{
	ObjectImpl<InfluxdbWriter>::Start(runtimeCreated);

	Log(LogInformation, "InfluxdbWriter")
	    << "'" << GetName() << "' started.";

	/* Register exception handler for WQ tasks. */
	m_WorkQueue.SetExceptionCallback(std::bind(&InfluxdbWriter::ExceptionHandler, this, _1));

	/* Setup timer for periodically flushing m_DataBuffer */
	m_FlushTimer = new Timer();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect(std::bind(&InfluxdbWriter::FlushTimeout, this));
	m_FlushTimer->Start();
	m_FlushTimer->Reschedule(0);

	/* Register for new metrics. */
	Service::OnNewCheckResult.connect(std::bind(&InfluxdbWriter::CheckResultHandler, this, _1, _2));
}

void InfluxdbWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "InfluxdbWriter")
	    << "'" << GetName() << "' stopped.";

	m_WorkQueue.Join();

	ObjectImpl<InfluxdbWriter>::Stop(runtimeRemoved);
}

void InfluxdbWriter::AssertOnWorkQueue(void)
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void InfluxdbWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "InfluxdbWriter", "Exception during InfluxDB operation: Verify that your backend is operational!");

	Log(LogDebug, "InfluxdbWriter")
	    << "Exception during InfluxDB operation: " << DiagnosticInformation(exp);

	//TODO: Close the connection, if we keep it open.
}

Stream::Ptr InfluxdbWriter::Connect()
{
	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "InfluxdbWriter")
	    << "Reconnecting to InfluxDB on host '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Can't connect to InfluxDB on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	if (GetSslEnable()) {
		boost::shared_ptr<SSL_CTX> sslContext;
		try {
			sslContext = MakeSSLContext(GetSslCert(), GetSslKey(), GetSslCaCert());
		} catch (const std::exception& ex) {
			Log(LogWarning, "InfluxdbWriter")
			    << "Unable to create SSL context.";
			throw ex;
		}

		TlsStream::Ptr tlsStream = new TlsStream(socket, GetHost(), RoleClient, sslContext);
		try {
			tlsStream->Handshake();
		} catch (const std::exception& ex) {
			Log(LogWarning, "InfluxdbWriter")
			    << "TLS handshake with host '" << GetHost() << "' failed.";
			throw ex;
		}

		return tlsStream;
	} else {
		return new NetworkStream(socket);
	}
}

void InfluxdbWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	m_WorkQueue.Enqueue(std::bind(&InfluxdbWriter::InternalCheckResultHandler, this, checkable, cr));
}

void InfluxdbWriter::InternalCheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	CONTEXT("Processing check result for '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	boost::tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	String prefix;

	double ts = cr->GetExecutionEnd();

	// Clone the template and perform an in-place macro expansion of measurement and tag values
	Dictionary::Ptr tmpl_clean = service ? GetServiceTemplate() : GetHostTemplate();
	Dictionary::Ptr tmpl = static_pointer_cast<Dictionary>(tmpl_clean->Clone());
	tmpl->Set("measurement", MacroProcessor::ResolveMacros(tmpl->Get("measurement"), resolvers, cr));

	Dictionary::Ptr tags = tmpl->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			// Prevent missing macros from warning; will return an empty value
			// which will be filtered out in SendMetric()
			String missing_macro;
			tags->Set(pair.first, MacroProcessor::ResolveMacros(pair.second, resolvers, cr, &missing_macro));
		}
	}

	SendPerfdata(tmpl, checkable, cr, ts);
}

String InfluxdbWriter::FormatInteger(int val)
{
	return Convert::ToString(val) + "i";
}

String InfluxdbWriter::FormatBoolean(bool val)
{
	return val ? "true" : "false";
}

void InfluxdbWriter::SendPerfdata(const Dictionary::Ptr& tmpl, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, double ts)
{
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
					Log(LogWarning, "InfluxdbWriter")
					    << "Ignoring invalid perfdata value: " << val;
					continue;
				}
			}

			Dictionary::Ptr fields = new Dictionary();
			fields->Set("value", pdv->GetValue());

			if (GetEnableSendThresholds()) {
				if (pdv->GetCrit())
					fields->Set("crit", pdv->GetCrit());
				if (pdv->GetWarn())
					fields->Set("warn", pdv->GetWarn());
				if (pdv->GetMin())
					fields->Set("min", pdv->GetMin());
				if (pdv->GetMax())
					fields->Set("max", pdv->GetMax());
			}
			if (!pdv->GetUnit().IsEmpty()) {
 				fields->Set("unit", pdv->GetUnit());
 			}

			SendMetric(tmpl, pdv->GetLabel(), fields, ts);
		}
	}

	if (GetEnableSendMetadata()) {
		Host::Ptr host;
		Service::Ptr service;
		boost::tie(host, service) = GetHostService(checkable);

		Dictionary::Ptr fields = new Dictionary();

		if (service)
			fields->Set("state", FormatInteger(service->GetState()));
		else
			fields->Set("state", FormatInteger(host->GetState()));

		fields->Set("current_attempt", FormatInteger(checkable->GetCheckAttempt()));
		fields->Set("max_check_attempts", FormatInteger(checkable->GetMaxCheckAttempts()));
		fields->Set("state_type", FormatInteger(checkable->GetStateType()));
		fields->Set("reachable", FormatBoolean(checkable->IsReachable()));
		fields->Set("downtime_depth", FormatInteger(checkable->GetDowntimeDepth()));
		fields->Set("acknowledgement", FormatInteger(checkable->GetAcknowledgement()));
		fields->Set("latency", cr->CalculateLatency());
		fields->Set("execution_time", cr->CalculateExecutionTime());

		SendMetric(tmpl, Empty, fields, ts);
	}
}

String InfluxdbWriter::EscapeKey(const String& str)
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
	size_t length = result.GetLength();
	if (result[length - 1] == '\\')
		result[length - 1] = '_';

	return result;
}

String InfluxdbWriter::EscapeField(const String& str)
{
	//TODO: Evaluate whether boost::regex is really needed here.

	// Handle integers
	boost::regex integer("-?\\d+i");
	if (boost::regex_match(str.GetData(), integer)) {
		return str;
	}

	// Handle numerics
	boost::regex numeric("-?\\d+(\\.\\d+)?((e|E)[+-]?\\d+)?");
	if (boost::regex_match(str.GetData(), numeric)) {
		return str;
	}

	// Handle booleans
	boost::regex boolean_true("t|true", boost::regex::icase);
	if (boost::regex_match(str.GetData(), boolean_true))
		return "true";
	boost::regex boolean_false("f|false", boost::regex::icase);
	if (boost::regex_match(str.GetData(), boolean_false))
		return "false";

	// Handle NaNs
	if (boost::math::isnan(str))
		return 0;

	// Otherwise it's a string and needs escaping and quoting
	String result = str;
	boost::algorithm::replace_all(result, "\"", "\\\"");
	return "\"" + result + "\"";
}

void InfluxdbWriter::SendMetric(const Dictionary::Ptr& tmpl, const String& label, const Dictionary::Ptr& fields, double ts)
{
	std::ostringstream msgbuf;
	msgbuf << EscapeKey(tmpl->Get("measurement"));

	Dictionary::Ptr tags = tmpl->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			// Empty macro expansion, no tag
			if (!pair.second.IsEmpty()) {
				msgbuf << "," << EscapeKey(pair.first) << "=" << EscapeKey(pair.second);
			}
		}
	}

	// Label is may be empty in the case of metadata
	if (!label.IsEmpty())
		msgbuf << ",metric=" << EscapeKey(label);

	msgbuf << " ";

	{
		bool first = true;

		ObjectLock fieldLock(fields);
		for (const Dictionary::Pair& pair : fields) {
			if (first)
				first = false;
			else
				msgbuf << ",";

			msgbuf << EscapeKey(pair.first) << "=" << EscapeField(pair.second);
		}
	}

	msgbuf << " " <<  static_cast<unsigned long>(ts);

	Log(LogDebug, "InfluxdbWriter")
	    << "Add to metric list: '" << msgbuf.str() << "'.";

	// Atomically buffer the data point
	boost::mutex::scoped_lock lock(m_DataBufferMutex);
	m_DataBuffer.push_back(String(msgbuf.str()));

	// Flush if we've buffered too much to prevent excessive memory use
	if (static_cast<int>(m_DataBuffer.size()) >= GetFlushThreshold()) {
		Log(LogDebug, "InfluxdbWriter")
		    << "Data buffer overflow writing " << m_DataBuffer.size() << " data points";
		Flush();
	}
}

void InfluxdbWriter::FlushTimeout(void)
{
	// Prevent new data points from being added to the array, there is a
	// race condition where they could disappear
	boost::mutex::scoped_lock lock(m_DataBufferMutex);

	// Flush if there are any data available
	if (m_DataBuffer.size() > 0) {
		Log(LogDebug, "InfluxdbWriter")
		    << "Timer expired writing " << m_DataBuffer.size() << " data points";
		Flush();
	}
}

void InfluxdbWriter::Flush(void)
{
	// Ensure you hold a lock against m_DataBuffer so that things
	// don't go missing after creating the body and clearing the buffer
	String body = boost::algorithm::join(m_DataBuffer, "\n");
	m_DataBuffer.clear();

	Stream::Ptr stream = Connect();

	if (!stream)
		return;

	Url::Ptr url = new Url();
	url->SetScheme(GetSslEnable() ? "https" : "http");
	url->SetHost(GetHost());
	url->SetPort(GetPort());

	std::vector<String> path;
	path.push_back("write");
	url->SetPath(path);

	url->AddQueryElement("db", GetDatabase());
	url->AddQueryElement("precision", "s");
	if (!GetUsername().IsEmpty())
		url->AddQueryElement("u", GetUsername());
	if (!GetPassword().IsEmpty())
		url->AddQueryElement("p", GetPassword());

	HttpRequest req(stream);
	req.RequestMethod = "POST";
	req.RequestUrl = url;

	try {
		req.WriteBody(body.CStr(), body.GetLength());
		req.Finish();
	} catch (const std::exception& ex) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	//TODO: Evaluate whether waiting for the result makes sense here. KeepAlive and close are options.
	HttpResponse resp(stream, req);
	StreamReadContext context;

	try {
		resp.Parse(context, true);
	} catch (const std::exception& ex) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Cannot read from TCP socket from host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	if (resp.StatusCode != 204) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Unexpected response code " << resp.StatusCode;

		// Finish parsing the headers and body
		while (!resp.Complete)
			resp.Parse(context, true);

		String contentType = resp.Headers->Get("content-type");
		if (contentType != "application/json") {
			Log(LogWarning, "InfluxdbWriter")
			    << "Unexpected Content-Type: " << contentType;
			return;
		}

		size_t responseSize = resp.GetBodySize();
		boost::scoped_array<char> buffer(new char[responseSize + 1]);
		resp.ReadBody(buffer.get(), responseSize);
		buffer.get()[responseSize] = '\0';

		Dictionary::Ptr jsonResponse;
		try {
			jsonResponse = JsonDecode(buffer.get());
		} catch (...) {
			Log(LogWarning, "InfluxdbWriter")
			    << "Unable to parse JSON response:\n" << buffer.get();
			return;
		}

		String error = jsonResponse->Get("error");

		Log(LogCritical, "InfluxdbWriter")
		    << "InfluxDB error message:\n" << error;
	}
}

void InfluxdbWriter::ValidateHostTemplate(const Dictionary::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<InfluxdbWriter>::ValidateHostTemplate(value, utils);

	String measurement = value->Get("measurement");
	if (!MacroProcessor::ValidateMacroString(measurement))
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("host_template")("measurement"), "Closing $ not found in macro format string '" + measurement + "'."));

	Dictionary::Ptr tags = value->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			if (!MacroProcessor::ValidateMacroString(pair.second))
				BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("host_template")("tags")(pair.first), "Closing $ not found in macro format string '" + pair.second));
		}
	}
}

void InfluxdbWriter::ValidateServiceTemplate(const Dictionary::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<InfluxdbWriter>::ValidateServiceTemplate(value, utils);

	String measurement = value->Get("measurement");
	if (!MacroProcessor::ValidateMacroString(measurement))
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("service_template")("measurement"), "Closing $ not found in macro format string '" + measurement + "'."));

	Dictionary::Ptr tags = value->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		for (const Dictionary::Pair& pair : tags) {
			if (!MacroProcessor::ValidateMacroString(pair.second))
				BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("service_template")("tags")(pair.first), "Closing $ not found in macro format string '" + pair.second));
		}
	}
}

