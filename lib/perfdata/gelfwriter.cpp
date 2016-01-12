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

#include "perfdata/gelfwriter.hpp"
#include "perfdata/gelfwriter.tcpp"
#include "icinga/service.hpp"
#include "icinga/notification.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/perfdatavalue.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
#include "base/json.hpp"
#include "base/context.hpp"

using namespace icinga;

REGISTER_TYPE(GelfWriter);

void GelfWriter::Start(bool runtimeCreated)
{
	ObjectImpl<GelfWriter>::Start(runtimeCreated);

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&GelfWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	// Send check results
	Service::OnNewCheckResult.connect(boost::bind(&GelfWriter::CheckResultHandler, this, _1, _2));
	// Send notifications
	Service::OnNotificationSentToUser.connect(boost::bind(&GelfWriter::NotificationToUserHandler, this, _1, _2, _3, _4, _5, _6, _7, _8));
	// Send state change
	Service::OnStateChange.connect(boost::bind(&GelfWriter::StateChangeHandler, this, _1, _2, _3));
}

void GelfWriter::ReconnectTimerHandler(void)
{
	if (m_Stream)
		return;

	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "GelfWriter")
	    << "Reconnecting to GELF endpoint '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (std::exception&) {
		Log(LogCritical, "GelfWriter")
		    << "Can't connect to GELF endpoint '" << GetHost() << "' port '" << GetPort() << "'.";
		return;
	}

	m_Stream = new NetworkStream(socket);
}

void GelfWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	CONTEXT("GELF Processing check result for '" + checkable->GetName() + "'");

	Log(LogDebug, "GelfWriter")
	    << "GELF Processing check result for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("_service_name", service->GetShortName());
		fields->Set("_service_state", Service::StateToString(service->GetState()));
		fields->Set("_last_state", service->GetLastState());
		fields->Set("_last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("_last_state", host->GetLastState());
		fields->Set("_last_hard_state", host->GetLastHardState());
	}

	fields->Set("_hostname", host->GetName());
	fields->Set("_type", "CHECK RESULT");
	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("_current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("_max_check_attempts", checkable->GetMaxCheckAttempts());

	if (cr) {
		fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("full_message", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("_check_source", cr->GetCheckSource());
	}

	SendLogMessage(ComposeGelfMessage(fields, GetSource()));
}

void GelfWriter::NotificationToUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
    const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
    const String& author, const String& comment_text, const String& command_name)
{
  	CONTEXT("GELF Processing notification to all users '" + checkable->GetName() + "'");

	Log(LogDebug, "GelfWriter")
	    << "GELF Processing notification for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

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
		fields->Set("_type", "SERVICE NOTIFICATION");
		fields->Set("_service", service->GetShortName());
		fields->Set("short_message", output);
	} else {
		fields->Set("_type", "HOST NOTIFICATION");
		fields->Set("short_message", "(" + CompatUtility::GetHostStateString(host) + ")");
	}

	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("_hostname", host->GetName());
	fields->Set("_command", command_name);
	fields->Set("_notification_type", notification_type_str);
	fields->Set("_comment", author_comment);

	SendLogMessage(ComposeGelfMessage(fields, GetSource()));
}

void GelfWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	CONTEXT("GELF Processing state change '" + checkable->GetName() + "'");

	Log(LogDebug, "GelfWriter")
	    << "GELF Processing state change for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));
	fields->Set("_type", "STATE CHANGE");
	fields->Set("_current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("_max_check_attempts", checkable->GetMaxCheckAttempts());
	fields->Set("_hostname", host->GetName());

	if (service) {
		fields->Set("_service_name", service->GetShortName());
		fields->Set("_service_state", Service::StateToString(service->GetState()));
		fields->Set("_last_state", service->GetLastState());
		fields->Set("_last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("_last_state", host->GetLastState());
		fields->Set("_last_hard_state", host->GetLastHardState());
	}

	if (cr) {
		fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("full_message", CompatUtility::GetCheckResultLongOutput(cr));
		fields->Set("_check_source", cr->GetCheckSource());
	}

	SendLogMessage(ComposeGelfMessage(fields, GetSource()));
}

String GelfWriter::ComposeGelfMessage(const Dictionary::Ptr& fields, const String& source)
{
	fields->Set("version", "1.1");
	fields->Set("host", source);
	fields->Set("timestamp", Utility::GetTime());

	return JsonEncode(fields);
}

void GelfWriter::SendLogMessage(const String& gelf)
{
	std::ostringstream msgbuf;
	msgbuf << gelf;
	msgbuf << '\0';

	String log = msgbuf.str();

	ObjectLock olock(this);

	if (!m_Stream)
		return;

	try {
		//TODO remove
		Log(LogDebug, "GelfWriter")
		    << "Sending '" << log << "'.";
		m_Stream->Write(log.CStr(), log.GetLength());
	} catch (const std::exception& ex) {
		Log(LogCritical, "GelfWriter")
		    << "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

		m_Stream.reset();
	}
}
