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

#include "perfdata/logstashwriter.hpp"
#include "perfdata/logstashwriter.tcpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/perfdatavalue.hpp"
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
	if(GetDefaultProtocol() == true)
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
        CONTEXT("LOGSTASGH Processing check result for '" + checkable->GetName() + "'");

        Log(LogDebug, "LogstashWriter")<< "Logstash Processing check result for '" << checkable->GetName() << "'";

        Host::Ptr host;
        Service::Ptr service;
        tie(host, service) = GetHostService(checkable);
        double ts = cr->GetExecutionEnd();

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

        fields->Set("hostname", host->GetName());
        fields->Set("type", "CHECK RESULT");
        fields->Set("state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

        fields->Set("current_check_attempt", checkable->GetCheckAttempt());
        fields->Set("max_check_attempts", checkable->GetMaxCheckAttempts());

        fields->Set("latency", cr->CalculateLatency());
        fields->Set("execution_time", cr->CalculateExecutionTime());
        fields->Set("reachable",  checkable->IsReachable());

        if (cr) {
		fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
                fields->Set("full_message", CompatUtility::GetCheckResultLongOutput(cr));
                fields->Set("check_source", cr->GetCheckSource());
        }

        if (GetEnableSendPerfdata()) {
                Array::Ptr perfdata = cr->GetPerformanceData();

                if (perfdata) {
                        ObjectLock olock(perfdata);
                        BOOST_FOREACH(const Value& val, perfdata) {
                                PerfdataValue::Ptr pdv;

                                if (val.IsObjectType<PerfdataValue>())
                                        pdv = val;
                                else {
                                        try {
                                                pdv = PerfdataValue::Parse(val);
					        String escaped_key = pdv->GetLabel();
						boost::replace_all(escaped_key, " ", "_");
                                                boost::replace_all(escaped_key, ".", "_");
                                                boost::replace_all(escaped_key, "\\", "_");
                                                boost::algorithm::replace_all(escaped_key, "::", ".");
			 			fields->Set(escaped_key, pdv->GetValue());

                                                if (pdv->GetMin())
                                                        fields->Set(escaped_key + "_min", pdv->GetMin());
                                                if (pdv->GetMax())
                                                        fields->Set(escaped_key + "_max", pdv->GetMax());
                                                if (pdv->GetWarn())
                                                        fields->Set(escaped_key + "_warn", pdv->GetWarn());
                                                if (pdv->GetCrit())
                                                        fields->Set(escaped_key + "_crit", pdv->GetCrit());
                                        } catch (const std::exception&) {
                                                Log(LogWarning, "LogstashWriter")
                                                    << "Ignoring invalid perfdata value: '" << val << "' for object '"
                                                    << checkable-GetName() << "'.";
                                        }
                                }
                        }
                }
        }

        SendLogMessage(ComposeLogstashMessage(fields, GetSource(), ts));
}


void LogstashWriter::NotificationToUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
    const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
    const String& author, const String& comment_text, const String& command_name)
{
        CONTEXT("Logstash Processing notification to all users '" + checkable->GetName() + "'");

        Log(LogDebug, "LogstashWriter")
            << "Logstash Processing notification for '" << checkable->GetName() << "'";

        Host::Ptr host;
        Service::Ptr service;
        tie(host, service) = GetHostService(checkable);
        double ts = cr->GetExecutionEnd();

        String notification_type_str = Notification::NotificationTypeToString(notification_type);

        String author_comment = "";

        if (notification_type == NotificationCustom || notification_type == NotificationAcknowledgement) {
                author_comment = author + ";" + comment_text;
        }

		String output;
        if (cr)
			output = CompatUtility::GetCheckResultOutput(cr);

        Dictionary::Ptr fields = new Dictionary();

        if (service) {
                fields->Set("type", "SERVICE NOTIFICATION");
                fields->Set("service", service->GetShortName());
                fields->Set("short_message", output);
        } else {
                fields->Set("type", "HOST NOTIFICATION");
                fields->Set("short_message", CompatUtility::GetHostStateString(host)+ ")");
        }

        fields->Set("state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

        fields->Set("hostname", host->GetName());
        fields->Set("command", command_name);
        fields->Set("notification_type", notification_type_str);
        fields->Set("comment", author_comment);

        SendLogMessage(ComposeLogstashMessage(fields, GetSource(), ts));
}

void LogstashWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
        CONTEXT("Logstash Processing state change '" + checkable->GetName() + "'");

        Log(LogDebug, "LogstashWriter")
            << "Logstash Processing state change for '" << checkable->GetName() << "'";

        Host::Ptr host;
        Service::Ptr service;
        tie(host, service) = GetHostService(checkable);
        double ts = cr->GetExecutionEnd();

        Dictionary::Ptr fields = new Dictionary();

        fields->Set("state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));
        fields->Set("type", "STATE CHANGE");
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

        if (cr) {
                fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
                fields->Set("full_message", CompatUtility::GetCheckResultLongOutput(cr));
                fields->Set("check_source", cr->GetCheckSource());
        }
        SendLogMessage(ComposeLogstashMessage(fields, GetSource(), ts));
}

String LogstashWriter::ComposeLogstashMessage(const Dictionary::Ptr& fields, const String& source, double ts)
{
        fields->Set("version", "1.1");
        fields->Set("host", source);
        fields->Set("timestamp", ts);
        String logstashObj= JsonEncode(fields);
	return logstashObj+ "\n";
}

void LogstashWriter::SendLogMessage(const String& message)
{
        ObjectLock olock(this);

        if (!m_Stream)
                return;

        try {
             m_Stream->Write(&message[0], message.GetLength());
        } catch (const std::exception& ex) {
                Log(LogCritical, "LogstashWriter") << "Cannot write to " << 
		((GetDefaultProtocol()==true) ? "tcp" : "udp") << " socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

                m_Stream.reset();
        }
}

