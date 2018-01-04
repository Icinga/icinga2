/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/application.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
#include "base/context.hpp"
#include "base/exception.hpp"
#include "base/json.hpp"
#include "base/statsfunction.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <utility>

using namespace icinga;

REGISTER_TYPE(GelfWriter);

REGISTER_STATSFUNCTION(GelfWriter, &GelfWriter::StatsFunc);

GelfWriter::GelfWriter()
	: m_WorkQueue(10000000, 1)
{ }

void GelfWriter::OnConfigLoaded()
{
	ObjectImpl<GelfWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("GelfWriter, " + GetName());
}

void GelfWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const GelfWriter::Ptr& gelfwriter : ConfigType::GetObjectsByType<GelfWriter>()) {
		size_t workQueueItems = gelfwriter->m_WorkQueue.GetLength();
		double workQueueItemRate = gelfwriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("work_queue_items", workQueueItems);
		stats->Set("work_queue_item_rate", workQueueItemRate);
		stats->Set("connected", gelfwriter->GetConnected());
		stats->Set("source", gelfwriter->GetSource());

		nodes->Set(gelfwriter->GetName(), stats);

		perfdata->Add(new PerfdataValue("gelfwriter_" + gelfwriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("gelfwriter_" + gelfwriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
	}

	status->Set("gelfwriter", nodes);
}

void GelfWriter::Start(bool runtimeCreated)
{
	ObjectImpl<GelfWriter>::Start(runtimeCreated);

	Log(LogInformation, "GelfWriter")
		<< "'" << GetName() << "' started.";

	/* Register exception handler for WQ tasks. */
	m_WorkQueue.SetExceptionCallback(std::bind(&GelfWriter::ExceptionHandler, this, _1));

	/* Timer for reconnecting */
	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(std::bind(&GelfWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	/* Register event handlers. */
	Checkable::OnNewCheckResult.connect(std::bind(&GelfWriter::CheckResultHandler, this, _1, _2));
	Checkable::OnNotificationSentToUser.connect(std::bind(&GelfWriter::NotificationToUserHandler, this, _1, _2, _3, _4, _5, _6, _7, _8));
	Checkable::OnStateChange.connect(std::bind(&GelfWriter::StateChangeHandler, this, _1, _2, _3));
}

void GelfWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "GelfWriter")
		<< "'" << GetName() << "' stopped.";

	m_WorkQueue.Join();

	ObjectImpl<GelfWriter>::Stop(runtimeRemoved);
}

void GelfWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void GelfWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "GelfWriter", "Exception during Graylog Gelf operation: Verify that your backend is operational!");

	Log(LogDebug, "GelfWriter")
		<< "Exception during Graylog Gelf operation: " << DiagnosticInformation(std::move(exp));

	if (GetConnected()) {
		m_Stream->Close();

		SetConnected(false);
	}
}

void GelfWriter::Reconnect()
{
	AssertOnWorkQueue();

	double startTime = Utility::GetTime();

	CONTEXT("Reconnecting to Graylog Gelf '" + GetName() + "'");

	SetShouldConnect(true);

	if (GetConnected())
		return;

	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "GelfWriter")
		<< "Reconnecting to Graylog Gelf on host '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogCritical, "GelfWriter")
			<< "Can't connect to Graylog Gelf on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	m_Stream = new NetworkStream(socket);

	SetConnected(true);

	Log(LogInformation, "GelfWriter")
		<< "Finished reconnecting to Graylog Gelf in " << std::setw(2) << Utility::GetTime() - startTime << " second(s).";
}

void GelfWriter::ReconnectTimerHandler()
{
	m_WorkQueue.Enqueue(std::bind(&GelfWriter::Reconnect, this), PriorityNormal);
}

void GelfWriter::Disconnect()
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	m_Stream->Close();

	SetConnected(false);
}

void GelfWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	m_WorkQueue.Enqueue(std::bind(&GelfWriter::CheckResultHandlerInternal, this, checkable, cr));
}

void GelfWriter::CheckResultHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	CONTEXT("GELF Processing check result for '" + checkable->GetName() + "'");

	Log(LogDebug, "GelfWriter")
		<< "Processing check result for '" << checkable->GetName() << "'";

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

	fields->Set("_reachable", checkable->IsReachable());

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	if (commandObj)
		fields->Set("_check_command", commandObj->GetName());

	double ts = Utility::GetTime();

	if (cr) {
		fields->Set("_latency", cr->CalculateLatency());
		fields->Set("_execution_time", cr->CalculateExecutionTime());
		fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("full_message", cr->GetOutput());
		fields->Set("_check_source", cr->GetCheckSource());
		ts = cr->GetExecutionEnd();
	}

	if (cr && GetEnableSendPerfdata()) {
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
						Log(LogWarning, "GelfWriter")
							<< "Ignoring invalid perfdata value: '" << val << "' for object '"
							<< checkable->GetName() << "'.";
					}
				}

				String escaped_key = pdv->GetLabel();
				boost::replace_all(escaped_key, " ", "_");
				boost::replace_all(escaped_key, ".", "_");
				boost::replace_all(escaped_key, "\\", "_");
				boost::algorithm::replace_all(escaped_key, "::", ".");

				fields->Set("_" + escaped_key, pdv->GetValue());

				if (pdv->GetMin())
					fields->Set("_" + escaped_key + "_min", pdv->GetMin());
				if (pdv->GetMax())
					fields->Set("_" + escaped_key + "_max", pdv->GetMax());
				if (pdv->GetWarn())
					fields->Set("_" + escaped_key + "_warn", pdv->GetWarn());
				if (pdv->GetCrit())
					fields->Set("_" + escaped_key + "_crit", pdv->GetCrit());
			}
		}
	}

	SendLogMessage(ComposeGelfMessage(fields, GetSource(), ts));
}

void GelfWriter::NotificationToUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
	const User::Ptr& user, NotificationType notificationType, CheckResult::Ptr const& cr,
	const String& author, const String& commentText, const String& commandName)
{
	m_WorkQueue.Enqueue(std::bind(&GelfWriter::NotificationToUserHandlerInternal, this,
		notification, checkable, user, notificationType, cr, author, commentText, commandName));
}

void GelfWriter::NotificationToUserHandlerInternal(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
	const User::Ptr& user, NotificationType notificationType, CheckResult::Ptr const& cr,
	const String& author, const String& commentText, const String& commandName)
{
	AssertOnWorkQueue();

	CONTEXT("GELF Processing notification to all users '" + checkable->GetName() + "'");

	Log(LogDebug, "GelfWriter")
		<< "Processing notification for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String notificationTypeString = Notification::NotificationTypeToString(notificationType);

	String authorComment = "";

	if (notificationType == NotificationCustom || notificationType == NotificationAcknowledgement) {
		authorComment = author + ";" + commentText;
	}

	String output;
	double ts = Utility::GetTime();

	if (cr) {
		output = CompatUtility::GetCheckResultOutput(cr);
		ts = cr->GetExecutionEnd();
	}

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("_type", "SERVICE NOTIFICATION");
		//TODO: fix this to _service_name
		fields->Set("_service", service->GetShortName());
		fields->Set("short_message", output);
	} else {
		fields->Set("_type", "HOST NOTIFICATION");
		//TODO: why?
		fields->Set("short_message", "(" + CompatUtility::GetHostStateString(host) + ")");
	}

	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("_hostname", host->GetName());
	fields->Set("_command", commandName);
	fields->Set("_notification_type", notificationTypeString);
	fields->Set("_comment", authorComment);

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	if (commandObj)
		fields->Set("_check_command", commandObj->GetName());

	SendLogMessage(ComposeGelfMessage(fields, GetSource(), ts));
}

void GelfWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	m_WorkQueue.Enqueue(std::bind(&GelfWriter::StateChangeHandlerInternal, this, checkable, cr, type));
}

void GelfWriter::StateChangeHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	AssertOnWorkQueue();

	CONTEXT("GELF Processing state change '" + checkable->GetName() + "'");

	Log(LogDebug, "GelfWriter")
		<< "Processing state change for '" << checkable->GetName() << "'";

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

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	if (commandObj)
		fields->Set("_check_command", commandObj->GetName());

	double ts = Utility::GetTime();

	if (cr) {
		fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("full_message", cr->GetOutput());
		fields->Set("_check_source", cr->GetCheckSource());
		ts = cr->GetExecutionEnd();
	}

	SendLogMessage(ComposeGelfMessage(fields, GetSource(), ts));
}

String GelfWriter::ComposeGelfMessage(const Dictionary::Ptr& fields, const String& source, double ts)
{
	fields->Set("version", "1.1");
	fields->Set("host", source);
	fields->Set("timestamp", ts);

	return JsonEncode(fields);
}

void GelfWriter::SendLogMessage(const String& gelfMessage)
{
	std::ostringstream msgbuf;
	msgbuf << gelfMessage;
	msgbuf << '\0';

	String log = msgbuf.str();

	ObjectLock olock(this);

	if (!GetConnected())
		return;

	try {
		Log(LogDebug, "GelfWriter")
			<< "Sending '" << log << "'.";

		m_Stream->Write(log.CStr(), log.GetLength());
	} catch (const std::exception& ex) {
		Log(LogCritical, "GelfWriter")
			<< "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

		throw ex;
	}
}
