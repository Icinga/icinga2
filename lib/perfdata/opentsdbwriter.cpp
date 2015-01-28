/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "perfdata/opentsdbwriter.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/perfdatavalue.hpp"
#include "base/tcpsocket.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

REGISTER_TYPE(OpenTsdbWriter);

REGISTER_STATSFUNCTION(OpenTsdbWriterStats, &OpenTsdbWriter::StatsFunc);

Value OpenTsdbWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const OpenTsdbWriter::Ptr& opentsdbwriter, DynamicType::GetObjectsByType<OpenTsdbWriter>()) {
		nodes->Set(opentsdbwriter->GetName(), 1); //add more stats
	}

	status->Set("opentsdbwriter", nodes);

	return 0;
}

void OpenTsdbWriter::Start(void)
{
	DynamicObject::Start();

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&OpenTsdbWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	Service::OnNewCheckResult.connect(boost::bind(&OpenTsdbWriter::CheckResultHandler, this, _1, _2));
}

void OpenTsdbWriter::ReconnectTimerHandler(void)
{
	if (m_Stream)
		return;

	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "OpenTsdbWriter")
		<< "Reconnect to OpenTSDB TSD on host '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (std::exception&) {
		Log(LogCritical, "OpenTsdbWriter")
			<< "Can't connect to OpenTSDB TSD on host '" << GetHost() << "' port '" << GetPort() << "'.";
		return;
	}

	m_Stream = new NetworkStream(socket);
}

void OpenTsdbWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	CONTEXT("Processing check result for '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Service::Ptr service = dynamic_pointer_cast<Service>(checkable);
	Host::Ptr host;

	if (service)
		host = service->GetHost();
	else
		host = static_pointer_cast<Host>(checkable);

	String hostName = host->GetName();

	String metric;
	std::map<String, String> tags;
	tags["host"] = hostName;

	if (service) {
		String serviceName = service->GetShortName();
		EscapeMetric(serviceName);
		metric = "icinga.service." + serviceName;

		SendMetric(metric + ".state", tags, service->GetState());
	} else {
		metric = "icinga.host";
		SendMetric(metric + ".state", tags, host->GetState());
	}

	SendMetric(metric + ".state_type", tags, checkable->GetStateType());
	SendMetric(metric + ".reachable", tags, checkable->IsReachable());
	SendMetric(metric + ".downtime_depth", tags, checkable->GetDowntimeDepth());

	SendPerfdata(metric, tags, cr);

	metric = "icinga.check";

	if (service) {
		tags["type"] = "service";
		String serviceName = service->GetShortName();
		EscapeTag(serviceName);
		tags["service"] = serviceName;
	} else {
		tags["type"] = "host";
	}

	SendMetric(metric + ".current_attempt", tags, checkable->GetCheckAttempt());
	SendMetric(metric + ".max_check_attempts", tags, checkable->GetMaxCheckAttempts());
	SendMetric(metric + ".latency", tags, Service::CalculateLatency(cr));
	SendMetric(metric + ".execution_time", tags, Service::CalculateExecutionTime(cr));
}

void OpenTsdbWriter::SendPerfdata(const String& metric, const std::map<String, String>& tags, const CheckResult::Ptr& cr)
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
				Log(LogWarning, "OpenTsdbWriter")
					<< "Ignoring invalid perfdata value: " << val;
				continue;
			}
		}

		String escaped_key = pdv->GetLabel();
		EscapeMetric(escaped_key);
		boost::algorithm::replace_all(escaped_key, "::", ".");

		SendMetric(metric + "." + escaped_key, tags, pdv->GetValue());

		if (pdv->GetCrit())
			SendMetric(metric + "." + escaped_key + "_crit", tags, pdv->GetCrit());
		if (pdv->GetWarn())
			SendMetric(metric + "." + escaped_key + "_warn", tags, pdv->GetWarn());
		if (pdv->GetMin())
			SendMetric(metric + "." + escaped_key + "_min", tags, pdv->GetMin());
		if (pdv->GetMax())
			SendMetric(metric + "." + escaped_key + "_max", tags, pdv->GetMax());
	}
}

void OpenTsdbWriter::SendMetric(const String& metric, const std::map<String, String>& tags, double value)
{
	String tags_string = "";
	BOOST_FOREACH(const Dictionary::Pair& tag, tags) {
		tags_string += " " + tag.first + "=" + Convert::ToString(tag.second);
	}

	std::ostringstream msgbuf;
	/*
	 * must be (http://opentsdb.net/docs/build/html/user_guide/writing.html)
	 * put <metric> <timestamp> <value> <tagk1=tagv1[ tagk2=tagv2 ...tagkN=tagvN]>
	 * "tags" must include at least one tag, we use "host=HOSTNAME"
	 */
	msgbuf << "put " << metric << " " << static_cast<long>(Utility::GetTime()) << " " << Convert::ToString(value) << " " << tags_string;

	Log(LogDebug, "OpenTsdbWriter")
		<< "Add to metric list:'" << msgbuf.str() << "'.";

	/* do not send \n to debug log */
	msgbuf << "\n";
	String put = msgbuf.str();

	ObjectLock olock(this);

	if (!m_Stream)
		return;

	try {
		m_Stream->Write(put.CStr(), put.GetLength());
	} catch (const std::exception& ex) {
		Log(LogCritical, "OpenTsdbWriter")
			<< "Cannot write to OpenTSDB TSD on host '" << GetHost() << "' port '" << GetPort() + "'.";

		m_Stream.reset();
	}
}

/* for metric and tag name rules, see
 * http://opentsdb.net/docs/build/html/user_guide/writing.html#metrics-and-tags
 */
String OpenTsdbWriter::EscapeTag(const String& str)
{
	String result = str;

	boost::replace_all(result, " ", "_");
	boost::replace_all(result, "\\", "_");

	return result;
}

String OpenTsdbWriter::EscapeMetric(const String& str)
{
	String result = str;

	boost::replace_all(result, " ", "_");
	boost::replace_all(result, ".", "_");
	boost::replace_all(result, "\\", "_");

	return result;
}
