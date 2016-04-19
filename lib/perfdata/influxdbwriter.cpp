/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include "icinga/compatutility.hpp"
#include "icinga/perfdatavalue.hpp"
#include "icinga/checkcommand.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include "base/tlsutility.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

REGISTER_TYPE(InfluxdbWriter);

REGISTER_STATSFUNCTION(InfluxdbWriter, &InfluxdbWriter::StatsFunc);

void InfluxdbWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const InfluxdbWriter::Ptr& influxdbwriter, ConfigType::GetObjectsByType<InfluxdbWriter>()) {
		nodes->Set(influxdbwriter->GetName(), 1); //add more stats
	}

	status->Set("influxdbwriter", nodes);
}

void InfluxdbWriter::Start(bool runtimeCreated)
{
	m_DataBuffer = new Array();

	ObjectImpl<InfluxdbWriter>::Start(runtimeCreated);

	m_FlushTimer = new Timer();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect(boost::bind(&InfluxdbWriter::FlushTimeout, this));
	m_FlushTimer->Start();
	m_FlushTimer->Reschedule(0);

	Service::OnNewCheckResult.connect(boost::bind(&InfluxdbWriter::CheckResultHandler, this, _1, _2));
}

Stream::Ptr InfluxdbWriter::Connect(void)
{
	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "InfluxdbWriter")
	    << "Reconnecting to InfluxDB on host '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (std::exception&) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Can't connect to InfluxDB on host '" << GetHost() << "' port '" << GetPort() << "'.";
		return Stream::Ptr();
	}

	if (GetSslEnable()) {
		boost::shared_ptr<SSL_CTX> ssl_context;
		try {
			ssl_context = MakeSSLContext(GetSslCert(), GetSslKey(), GetSslCaCert());
		} catch (std::exception&) {
			Log(LogWarning, "InfluxdbWriter")
			    << "Unable to create SSL context.";
			return Stream::Ptr();
		}

		TlsStream::Ptr tls_stream = new TlsStream(socket, GetHost(), RoleClient, ssl_context);
		try {
			tls_stream->Handshake();
		} catch (std::exception&) {
			Log(LogWarning, "InfluxdbWriter")
			    << "TLS handshake with host '" << GetHost() << "' failed.";
			return Stream::Ptr();
		}

		return tls_stream;
	} else {
		return new NetworkStream(socket);
	}
}

void InfluxdbWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
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
	// Work Needed: Escape ' ', ',' and '=' in field keys, tag keys and tag values
	//              Quote field values when the type is string
	Dictionary::Ptr tmpl_clean = service ? GetServiceTemplate() : GetHostTemplate();
	Dictionary::Ptr tmpl = static_pointer_cast<Dictionary>(tmpl_clean->Clone());
	tmpl->Set("measurement", MacroProcessor::ResolveMacros(tmpl->Get("measurement"), resolvers, cr));

	Dictionary::Ptr tags = tmpl->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
retry:
		BOOST_FOREACH(const Dictionary::Pair& pair, tags) {
			// Prevent missing macros from warning; will return an empty value
			// which will be filtered out in SendMetric()
			String missing_macro;
			tags->Set(pair.first,  MacroProcessor::ResolveMacros(pair.second, resolvers, cr, &missing_macro));
		}
	}

	// If the service was appiled via a 'apply Service for' command then resolve the
	// instance and add it as a tag (e.g. check_command = mtu, name = mtueth0, instance = eth0)
	if (service && (service->GetName() != service->GetCheckCommand()->GetName())) {
		tags->Set("instance", service->GetName().SubStr(service->GetCheckCommand()->GetName().GetLength()));
	}

	SendPerfdata(tmpl, cr, ts);
}

void InfluxdbWriter::SendPerfdata(const Dictionary::Ptr tmpl, const CheckResult::Ptr& cr, double ts)
{
	Array::Ptr perfdata = cr->GetPerformanceData();

	if (!perfdata)
		return;

	ObjectLock olock(perfdata);
	BOOST_FOREACH(const Value& val, perfdata) {
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

		SendMetric(tmpl, pdv->GetLabel(), "value", pdv->GetValue(), ts);

		if (GetEnableSendThresholds()) {
			if (pdv->GetCrit())
				SendMetric(tmpl, pdv->GetLabel(), "crit", pdv->GetCrit(), ts);
			if (pdv->GetWarn())
				SendMetric(tmpl, pdv->GetLabel(), "warn", pdv->GetWarn(), ts);
			if (pdv->GetMin())
				SendMetric(tmpl, pdv->GetLabel(), "min", pdv->GetMin(), ts);
			if (pdv->GetMax())
				SendMetric(tmpl, pdv->GetLabel(), "max", pdv->GetMax(), ts);
		}
	}
}

void InfluxdbWriter::SendMetric(const Dictionary::Ptr tmpl, const String& label, const String& type, double value, double ts)
{
	std::ostringstream msgbuf;
	msgbuf << tmpl->Get("measurement");

	Dictionary::Ptr tags = tmpl->Get("tags");
	if (tags) {
		ObjectLock olock(tags);
		BOOST_FOREACH(const Dictionary::Pair& pair, tags) {
			// Empty macro expansion, no tag
			if (!pair.second.IsEmpty())
				msgbuf << "," << pair.first << "=" << pair.second;
		}
	}

	msgbuf << ",metric=" << label << ",type=" << type << " value=" << value << " " << static_cast<unsigned long>(ts);

	Log(LogDebug, "InfluxdbWriter")
	    << "Add to metric list:'" << msgbuf.str() << "'.";

	// Atomically buffer the data point
	ObjectLock olock(m_DataBuffer);
	m_DataBuffer->Add(String(msgbuf.str()));

	// Flush if we've buffered too much to prevent excessive memory use
	if (m_DataBuffer->GetLength() >= GetFlushThreshold()) {
		Log(LogDebug, "InfluxdbWriter")
		    << "Data buffer overflow writing " << m_DataBuffer->GetLength() << " data points";
		Flush();
	}
}

void InfluxdbWriter::FlushTimeout(void)
{
	// Prevent new data points from being added to the array, there is a
	// race condition where they could disappear
	ObjectLock olock(m_DataBuffer);

	// Flush if there are any data available
	if (m_DataBuffer->GetLength() > 0) {
		Log(LogDebug, "InfluxdbWriter")
		    << "Timer expired writing " << m_DataBuffer->GetLength() << " data points";
		Flush();
	}
}

void InfluxdbWriter::Flush(void)
{
	Stream::Ptr stream = Connect();

	// Unable to connect, play it safe and lose the data points
	// to avoid a memory leak
	if (!stream.get()) {
		m_DataBuffer->Clear();
		return;
	}

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

	// Ensure you hold a lock against m_DataBuffer so that things
	// don't go missing after creating the body and clearing the buffer
	String body = Utility::Join(m_DataBuffer, '\n');
	m_DataBuffer->Clear();

	HttpRequest req(stream);
	req.RequestMethod = "POST";
	req.RequestUrl = url;

	try {
		req.WriteBody(body.CStr(), body.GetLength());
		req.Finish();
	} catch (const std::exception&) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";
	}

	HttpResponse resp(stream, req);
	StreamReadContext context;

	try {
		resp.Parse(context, true);
	} catch (const std::exception) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Cannot read from TCP socket from host '" << GetHost() << "' port '" << GetPort() << "'.";
	}

	if (resp.StatusCode != 204) {
		Log(LogWarning, "InfluxdbWriter")
		    << "Unexpected response code " << resp.StatusCode;
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
		BOOST_FOREACH(const Dictionary::Pair& pair, tags) {
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
		BOOST_FOREACH(const Dictionary::Pair& pair, tags) {
			if (!MacroProcessor::ValidateMacroString(pair.second))
				BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of<String>("service_template")("tags")(pair.first), "Closing $ not found in macro format string '" + pair.second));
		}
	}
}

