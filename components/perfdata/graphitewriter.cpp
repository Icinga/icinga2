/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "perfdata/graphitewriter.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "icinga/compatutility.h"
#include "icinga/perfdatavalue.h"
#include "base/tcpsocket.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/application.h"
#include "base/stream.h"
#include "base/networkstream.h"
#include "base/bufferedstream.h"
#include "base/exception.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

REGISTER_TYPE(GraphiteWriter);

void GraphiteWriter::Start(void)
{
	DynamicObject::Start();

	m_ReconnectTimer = make_shared<Timer>();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&GraphiteWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	Service::OnNewCheckResult.connect(boost::bind(&GraphiteWriter::CheckResultHandler, this, _1, _2));
}

void GraphiteWriter::ReconnectTimerHandler(void)
{
	try {
		if (m_Stream) {
			m_Stream->Write("\n", 1);
			Log(LogWarning, "perfdata", "GraphiteWriter already connected on socket on host '" + GetHost() + "' port '" + GetPort() + "'.");
			return;
		}
	} catch (const std::exception& ex) {
		Log(LogWarning, "perfdata", "GraphiteWriter socket on host '" + GetHost() + "' port '" + GetPort() + "' gone. Attempting to reconnect.");	
	}

	TcpSocket::Ptr socket = make_shared<TcpSocket>();

	Log(LogDebug, "perfdata", "GraphiteWriter: Reconnect to tcp socket on host '" + GetHost() + "' port '" + GetPort() + "'.");
	socket->Connect(GetHost(), GetPort());

	NetworkStream::Ptr net_stream = make_shared<NetworkStream>(socket);
	m_Stream = make_shared<BufferedStream>(net_stream);
}

void GraphiteWriter::CheckResultHandler(const Service::Ptr& service, const CheckResult::Ptr& cr)
{
	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !service->GetEnablePerfdata())
		return;

	/* TODO: sanitize host and service names */
	String hostName = service->GetHost()->GetName();
	String serviceName = service->GetShortName();   

	SanitizeMetric(hostName);
	SanitizeMetric(serviceName);

	String prefix = "icinga." + hostName + "." + serviceName;

	/* basic metrics */
	SendMetric(prefix, "current_attempt", service->GetCheckAttempt());
	SendMetric(prefix, "max_check_attempts", service->GetMaxCheckAttempts());
	SendMetric(prefix, "state_type", service->GetStateType());
	SendMetric(prefix, "state", service->GetState());
	SendMetric(prefix, "latency", Service::CalculateLatency(cr));
	SendMetric(prefix, "execution_time", Service::CalculateExecutionTime(cr));

	Value pdv = cr->GetPerformanceData();

	if (!pdv.IsObjectType<Dictionary>())
		return;

	Dictionary::Ptr perfdata = pdv;

	ObjectLock olock(perfdata);
	BOOST_FOREACH(const Dictionary::Pair& kv, perfdata) {
		double valueNum;

		if (!kv.second.IsObjectType<PerfdataValue>())
			valueNum = kv.second;
		else
			valueNum = static_cast<PerfdataValue::Ptr>(kv.second)->GetValue();

		String escaped_key = kv.first;
		SanitizeMetric(escaped_key);
		boost::algorithm::replace_all(escaped_key, "::", ".");

		SendMetric(prefix, escaped_key, valueNum);
	}
}

void GraphiteWriter::SendMetric(const String& prefix, const String& name, double value)
{
	std::ostringstream msgbuf;
	msgbuf << prefix << "." << name << " " << value << " " << static_cast<long>(Utility::GetTime()) << "\n";

	String metric = msgbuf.str();
	Log(LogDebug, "perfdata", "GraphiteWriter: Add to metric list:'" + metric + "'.");

	ObjectLock olock(this);

	if (!m_Stream)
		return;

	try {
		m_Stream->Write(metric.CStr(), metric.GetLength());
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Exception thrown while writing to the Graphite socket: " << std::endl
		       << DiagnosticInformation(ex);

		Log(LogCritical, "base", msgbuf.str());

		m_Stream.reset();
	}
}

void GraphiteWriter::SanitizeMetric(String& str)
{
	boost::replace_all(str, " ", "_");
	boost::replace_all(str, ".", "_");
	boost::replace_all(str, "-", "_");
	boost::replace_all(str, "\\", "_");
	boost::replace_all(str, "/", "_");
}
