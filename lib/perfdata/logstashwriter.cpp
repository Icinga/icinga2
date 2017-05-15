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

#include "perfdata/logstashwriter.hpp"
#include "perfdata/logstashwriter.tcpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/notification.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
#include "base/json.hpp"
#include "base/context.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <string>

using namespace icinga;

REGISTER_TYPE(LogstashWriter);

void LogstashWriter::Start(bool runtimeCreated)
{
	ObjectImpl<LogstashWriter>::Start(runtimeCreated);

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&LogstashWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	// Send check results
	Service::OnNewCheckResult.connect(boost::bind(&LogstashWriter::CheckResultHandler, this, _1, _2));
	// Send notifications
	Service::OnNotificationSentToUser.connect(boost::bind(&LogstashWriter::NotificationToUserHandler, this, _1, _2, _3, _4, _5, _6, _7, _8));
	// Send state change
	Service::OnStateChange.connect(boost::bind(&LogstashWriter::StateChangeHandler, this, _1, _2, _3));
}

void LogstashWriter::ReconnectTimerHandler(void)
{
 	if (m_Stream)
		return;

	Socket::Ptr socket;

	if (GetSocketType() == "tcp")
		socket = new TcpSocket();
	else
		socket = new UdpSocket();

	Log(LogNotice, "LogstashWriter")
	    << "Reconnecting to Logstash endpoint '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (const std::exception&) {
		Log(LogCritical, "LogstashWriter")
		    << "Can't connect to Logstash endpoint '" << GetHost() << "' port '" << GetPort() << "'.";
		return;
	}

	m_Stream = new NetworkStream(socket);
}

void LogstashWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	CONTEXT("LOGSTASH Processing check result for '" + checkable->GetName() + "'");

	Log(LogDebug, "LogstashWriter")
	    << "Processing check result for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("service_name", service->GetShortName());
		fields->Set("service_state", Service::StateToString(service->GetState()));
		fields->Set("last_state", service->GetLastState());
		fields->Set("last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("last_state", host->GetLastState());
		fields->Set("last_hard_state", host->GetLastHardState());
	}

	fields->Set("host_name", host->GetName());
	fields->Set("type", "CheckResult");
	fields->Set("state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("max_check_attempts", checkable->GetMaxCheckAttempts());

	fields->Set("latency", cr->CalculateLatency());
	fields->Set("execution_time", cr->CalculateExecutionTime());
	fields->Set("reachable",  checkable->IsReachable());

	double ts = Utility::GetTime();

	if (cr) {
		fields->Set("plugin_output", cr->GetOutput());
		fields->Set("check_source", cr->GetCheckSource());
		ts = cr->GetExecutionEnd();
	}

	Array::Ptr perfdata = cr->GetPerformanceData();

	if (perfdata) {
		Dictionary::Ptr perfdataItems = new Dictionary();

		ObjectLock olock(perfdata);
		for (const Value& val : perfdata) {
			PerfdataValue::Ptr pdv;

			if (val.IsObjectType<PerfdataValue>())
				pdv = val;
			else {
				try {
					pdv = PerfdataValue::Parse(val);
				} catch (const std::exception&) {
					Log(LogWarning, "LogstashWriter")
					    << "Ignoring invalid perfdata value: '" << val << "' for object '"
					    << checkable->GetName() << "'.";
					continue;
				}
			}

			Dictionary::Ptr perfdataItem = new Dictionary();
			perfdataItem->Set("value", pdv->GetValue());

			if (pdv->GetMin())
				perfdataItem->Set("min", pdv->GetMin());
			if (pdv->GetMax())
				perfdataItem->Set("max", pdv->GetMax());
			if (pdv->GetWarn())
				perfdataItem->Set("warn", pdv->GetWarn());
			if (pdv->GetCrit())
				perfdataItem->Set("crit", pdv->GetCrit());

			String escaped_key = EscapeMetricLabel(pdv->GetLabel());

			perfdataItems->Set(escaped_key, perfdataItem);
		}

		fields->Set("performance_data", perfdataItems);
	}

	SendLogMessage(ComposeLogstashMessage(fields, GetSource(), ts));
}


void LogstashWriter::NotificationToUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
    const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
    const String& author, const String& comment_text, const String& command_name)
{
	CONTEXT("Logstash Processing notification to all users '" + checkable->GetName() + "'");

	Log(LogDebug, "LogstashWriter")
	    << "Processing notification for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String notification_type_str = Notification::NotificationTypeToString(notification_type);

	String author_comment = "";

	if (notification_type == NotificationCustom || notification_type == NotificationAcknowledgement) {
		author_comment = author + ";" + comment_text;
	}

	double ts = Utility::GetTime();

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("type", "SERVICE NOTIFICATION");
		fields->Set("service_name", service->GetShortName());
	} else {
		fields->Set("type", "HOST NOTIFICATION");
	}

	if (cr) {
		fields->Set("plugin_output", cr->GetOutput());
		ts = cr->GetExecutionEnd();
	}

	fields->Set("state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("host_name", host->GetName());
	fields->Set("command", command_name);
	fields->Set("notification_type", notification_type_str);
	fields->Set("comment", author_comment);

	SendLogMessage(ComposeLogstashMessage(fields, GetSource(), ts));
}

void LogstashWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	CONTEXT("Logstash Processing state change '" + checkable->GetName() + "'");

	Log(LogDebug, "LogstashWriter")
	    << "Processing state change for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	fields->Set("state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));
	fields->Set("type", "StateChange");
	fields->Set("current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("max_check_attempts", checkable->GetMaxCheckAttempts());
	fields->Set("hostname", host->GetName());

	if (service) {
		fields->Set("service_name", service->GetShortName());
		fields->Set("service_state", Service::StateToString(service->GetState()));
		fields->Set("last_state", service->GetLastState());
		fields->Set("last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("last_state", host->GetLastState());
		fields->Set("last_hard_state", host->GetLastHardState());
        }

	double ts = Utility::GetTime();

	if (cr) {
		fields->Set("plugin_output", cr->GetOutput());
		fields->Set("check_source", cr->GetCheckSource());
		ts = cr->GetExecutionEnd();
	}

	SendLogMessage(ComposeLogstashMessage(fields, GetSource(), ts));
}

String LogstashWriter::ComposeLogstashMessage(const Dictionary::Ptr& fields, const String& source, double ts)
{
        fields->Set("version", "1.1");
        fields->Set("host", source);
        fields->Set("timestamp", ts);

        return JsonEncode(fields) + "\n";
}

void LogstashWriter::SendLogMessage(const String& message)
{
        ObjectLock olock(this);

        if (!m_Stream)
                return;

        try {
		m_Stream->Write(&message[0], message.GetLength());
        } catch (const std::exception& ex) {
                Log(LogCritical, "LogstashWriter")
		    << "Cannot write to " << GetSocketType()
		    << " socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

                m_Stream.reset();
        }
}

String LogstashWriter::EscapeMetricLabel(const String& str)
{
	String result = str;

	boost::replace_all(result, " ", "_");
	boost::replace_all(result, ".", "_");
	boost::replace_all(result, "\\", "_");
	boost::replace_all(result, "::", ".");

	return result;
}

void LogstashWriter::ValidateSocketType(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<LogstashWriter>::ValidateSocketType(value, utils);

	if (value != "udp" && value != "tcp")
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("socket_type"), "Socket type '" + value + "' is invalid."));
}
