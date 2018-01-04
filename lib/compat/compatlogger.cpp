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

#include "compat/compatlogger.hpp"
#include "compat/compatlogger.tcpp"
#include "icinga/service.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notification.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/externalcommandprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include "base/statsfunction.hpp"
#include <boost/algorithm/string.hpp>

using namespace icinga;

REGISTER_TYPE(CompatLogger);

REGISTER_STATSFUNCTION(CompatLogger, &CompatLogger::StatsFunc);

void CompatLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const CompatLogger::Ptr& compat_logger : ConfigType::GetObjectsByType<CompatLogger>()) {
		nodes->Set(compat_logger->GetName(), 1); //add more stats
	}

	status->Set("compatlogger", nodes);
}

/**
 * @threadsafety Always.
 */
void CompatLogger::Start(bool runtimeCreated)
{
	ObjectImpl<CompatLogger>::Start(runtimeCreated);

	Log(LogInformation, "CompatLogger")
		<< "'" << GetName() << "' started.";

	Checkable::OnNewCheckResult.connect(std::bind(&CompatLogger::CheckResultHandler, this, _1, _2));
	Checkable::OnNotificationSentToUser.connect(std::bind(&CompatLogger::NotificationSentHandler, this, _1, _2, _3, _4, _5, _6, _7, _8));
	Downtime::OnDowntimeTriggered.connect(std::bind(&CompatLogger::TriggerDowntimeHandler, this, _1));
	Downtime::OnDowntimeRemoved.connect(std::bind(&CompatLogger::RemoveDowntimeHandler, this, _1));
	Checkable::OnEventCommandExecuted.connect(std::bind(&CompatLogger::EventCommandHandler, this, _1));

	Checkable::OnFlappingChanged.connect(std::bind(&CompatLogger::FlappingChangedHandler, this, _1));
	Checkable::OnEnableFlappingChanged.connect(std::bind(&CompatLogger::EnableFlappingChangedHandler, this, _1));

	ExternalCommandProcessor::OnNewExternalCommand.connect(std::bind(&CompatLogger::ExternalCommandHandler, this, _2, _3));

	m_RotationTimer = new Timer();
	m_RotationTimer->OnTimerExpired.connect(std::bind(&CompatLogger::RotationTimerHandler, this));
	m_RotationTimer->Start();

	ReopenFile(false);
	ScheduleNextRotation();
}

/**
 * @threadsafety Always.
 */
void CompatLogger::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "CompatLogger")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<CompatLogger>::Stop(runtimeRemoved);
}

/**
 * @threadsafety Always.
 */
void CompatLogger::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr &cr)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr vars_after = cr->GetVarsAfter();

	long state_after = vars_after->Get("state");
	long stateType_after = vars_after->Get("state_type");
	long attempt_after = vars_after->Get("attempt");
	bool reachable_after = vars_after->Get("reachable");

	Dictionary::Ptr vars_before = cr->GetVarsBefore();

	if (vars_before) {
		long state_before = vars_before->Get("state");
		long stateType_before = vars_before->Get("state_type");
		long attempt_before = vars_before->Get("attempt");
		bool reachable_before = vars_before->Get("reachable");

		if (state_before == state_after && stateType_before == stateType_after &&
			attempt_before == attempt_after && reachable_before == reachable_after)
			return; /* Nothing changed, ignore this checkresult. */
	}

	String output;
	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< Service::StateToString(service->GetState()) << ";"
			<< Service::StateTypeToString(service->GetStateType()) << ";"
			<< attempt_after << ";"
			<< output << ""
			<< "";
	} else {
		String state = Host::StateToString(Host::CalculateState(static_cast<ServiceState>(state_after)));

		msgbuf << "HOST ALERT: "
			<< host->GetName() << ";"
			<< CompatUtility::GetHostStateString(host) << ";"
			<< Host::StateTypeToString(host->GetStateType()) << ";"
			<< attempt_after << ";"
			<< output << ""
			<< "";

	}

	{
		ObjectLock olock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

/**
 * @threadsafety Always.
 */
void CompatLogger::TriggerDowntimeHandler(const Downtime::Ptr& downtime)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(downtime->GetCheckable());

	if (!downtime)
		return;

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< "STARTED" << "; "
			<< "Checkable has entered a period of scheduled downtime."
			<< "";
	} else {
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< "STARTED" << "; "
			<< "Checkable has entered a period of scheduled downtime."
			<< "";
	}

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

/**
 * @threadsafety Always.
 */
void CompatLogger::RemoveDowntimeHandler(const Downtime::Ptr& downtime)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(downtime->GetCheckable());

	if (!downtime)
		return;

	String downtime_output;
	String downtime_state_str;

	if (downtime->GetWasCancelled()) {
		downtime_output = "Scheduled downtime for service has been cancelled.";
		downtime_state_str = "CANCELLED";
	} else {
		downtime_output = "Checkable has exited from a period of scheduled downtime.";
		downtime_state_str = "STOPPED";
	}

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< downtime_state_str << "; "
			<< downtime_output
			<< "";
	} else {
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< downtime_state_str << "; "
			<< downtime_output
			<< "";
	}

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

/**
 * @threadsafety Always.
 */
void CompatLogger::NotificationSentHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
	const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
	const String& author, const String& comment_text, const String& command_name)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String notification_type_str = Notification::NotificationTypeToString(notification_type);

	/* override problem notifications with their current state string */
	if (notification_type == NotificationProblem) {
		if (service)
			notification_type_str = Service::StateToString(service->GetState());
		else
			notification_type_str = CompatUtility::GetHostStateString(host);
	}

	String author_comment = "";
	if (notification_type == NotificationCustom || notification_type == NotificationAcknowledgement) {
		author_comment = author + ";" + comment_text;
	}

	if (!cr)
		return;

	String output;
	if (cr)
		output = CompatUtility::GetCheckResultOutput(cr);

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE NOTIFICATION: "
			<< user->GetName() << ";"
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< notification_type_str << ";"
			<< command_name << ";"
			<< output << ";"
			<< author_comment
			<< "";
	} else {
		msgbuf << "HOST NOTIFICATION: "
			<< user->GetName() << ";"
			<< host->GetName() << ";"
			<< notification_type_str << " "
			<< "(" << CompatUtility::GetHostStateString(host) << ");"
			<< command_name << ";"
			<< output << ";"
			<< author_comment
			<< "";
	}

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

/**
 * @threadsafety Always.
 */
void CompatLogger::FlappingChangedHandler(const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String flapping_state_str;
	String flapping_output;

	if (checkable->IsFlapping()) {
		flapping_output = "Checkable appears to have started flapping (" + Convert::ToString(checkable->GetFlappingCurrent()) + "% change >= " + Convert::ToString(checkable->GetFlappingThresholdHigh()) + "% threshold)";
		flapping_state_str = "STARTED";
	} else {
		flapping_output = "Checkable appears to have stopped flapping (" + Convert::ToString(checkable->GetFlappingCurrent()) + "% change < " + Convert::ToString(checkable->GetFlappingThresholdLow()) + "% threshold)";
		flapping_state_str = "STOPPED";
	}

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	} else {
		msgbuf << "HOST FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	}

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

void CompatLogger::EnableFlappingChangedHandler(const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (checkable->GetEnableFlapping())
		return;

	String flapping_output = "Flap detection has been disabled";
	String flapping_state_str = "DISABLED";

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	} else {
		msgbuf << "HOST FLAPPING ALERT: "
			<< host->GetName() << ";"
			<< flapping_state_str << "; "
			<< flapping_output
			<< "";
	}

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

void CompatLogger::ExternalCommandHandler(const String& command, const std::vector<String>& arguments)
{
	std::ostringstream msgbuf;
	msgbuf << "EXTERNAL COMMAND: "
		<< command << ";"
		<< boost::algorithm::join(arguments, ";")
		<< "";

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

void CompatLogger::EventCommandHandler(const Checkable::Ptr& checkable)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	EventCommand::Ptr event_command = checkable->GetEventCommand();
	String event_command_name = event_command->GetName();
	long current_attempt = checkable->GetCheckAttempt();

	std::ostringstream msgbuf;

	if (service) {
		msgbuf << "SERVICE EVENT HANDLER: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< Service::StateToString(service->GetState()) << ";"
			<< Service::StateTypeToString(service->GetStateType()) << ";"
			<< current_attempt << ";"
			<< event_command_name;
	} else {
		msgbuf << "HOST EVENT HANDLER: "
			<< host->GetName() << ";"
			<< CompatUtility::GetHostStateString(host) << ";"
			<< Host::StateTypeToString(host->GetStateType()) << ";"
			<< current_attempt << ";"
			<< event_command_name;
	}

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
		Flush();
	}
}

void CompatLogger::WriteLine(const String& line)
{
	ASSERT(OwnsLock());

	if (!m_OutputFile.good())
		return;

	m_OutputFile << "[" << (long)Utility::GetTime() << "] " << line << "\n";
}

void CompatLogger::Flush()
{
	ASSERT(OwnsLock());

	if (!m_OutputFile.good())
		return;

	m_OutputFile << std::flush;
}

/**
 * @threadsafety Always.
 */
void CompatLogger::ReopenFile(bool rotate)
{
	ObjectLock olock(this);

	String tempFile = GetLogDir() + "/icinga.log";

	if (m_OutputFile) {
		m_OutputFile.close();

		if (rotate) {
			String archiveFile = GetLogDir() + "/archives/icinga-" + Utility::FormatDateTime("%m-%d-%Y-%H", Utility::GetTime()) + ".log";

			Log(LogNotice, "CompatLogger")
				<< "Rotating compat log file '" << tempFile << "' -> '" << archiveFile << "'";

			(void) rename(tempFile.CStr(), archiveFile.CStr());
		}
	}

	m_OutputFile.open(tempFile.CStr(), std::ofstream::app);

	if (!m_OutputFile) {
		Log(LogWarning, "CompatLogger")
			<< "Could not open compat log file '" << tempFile << "' for writing. Log output will be lost.";

		return;
	}

	WriteLine("LOG ROTATION: " + GetRotationMethod());
	WriteLine("LOG VERSION: 2.0");

	for (const Host::Ptr& host : ConfigType::GetObjectsByType<Host>()) {
		String output;
		CheckResult::Ptr cr = host->GetLastCheckResult();

		if (cr)
			output = CompatUtility::GetCheckResultOutput(cr);

		std::ostringstream msgbuf;
		msgbuf << "CURRENT HOST STATE: "
			<< host->GetName() << ";"
			<< CompatUtility::GetHostStateString(host) << ";"
			<< Host::StateTypeToString(host->GetStateType()) << ";"
			<< host->GetCheckAttempt() << ";"
			<< output << "";

		WriteLine(msgbuf.str());
	}

	for (const Service::Ptr& service : ConfigType::GetObjectsByType<Service>()) {
		Host::Ptr host = service->GetHost();

		String output;
		CheckResult::Ptr cr = service->GetLastCheckResult();

		if (cr)
			output = CompatUtility::GetCheckResultOutput(cr);

		std::ostringstream msgbuf;
		msgbuf << "CURRENT SERVICE STATE: "
			<< host->GetName() << ";"
			<< service->GetShortName() << ";"
			<< Service::StateToString(service->GetState()) << ";"
			<< Service::StateTypeToString(service->GetStateType()) << ";"
			<< service->GetCheckAttempt() << ";"
			<< output << "";

		WriteLine(msgbuf.str());
	}

	Flush();
}

void CompatLogger::ScheduleNextRotation()
{
	auto now = (time_t)Utility::GetTime();
	String method = GetRotationMethod();

	tm tmthen;

#ifdef _MSC_VER
	tm *temp = localtime(&now);

	if (!temp) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("localtime")
			<< boost::errinfo_errno(errno));
	}

	tmthen = *temp;
#else /* _MSC_VER */
	if (!localtime_r(&now, &tmthen)) {
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("localtime_r")
			<< boost::errinfo_errno(errno));
	}
#endif /* _MSC_VER */

	tmthen.tm_min = 0;
	tmthen.tm_sec = 0;

	if (method == "HOURLY") {
		tmthen.tm_hour++;
	} else if (method == "DAILY") {
		tmthen.tm_mday++;
		tmthen.tm_hour = 0;
	} else if (method == "WEEKLY") {
		tmthen.tm_mday += 7 - tmthen.tm_wday;
		tmthen.tm_hour = 0;
	} else if (method == "MONTHLY") {
		tmthen.tm_mon++;
		tmthen.tm_mday = 1;
		tmthen.tm_hour = 0;
	}

	time_t ts = mktime(&tmthen);

	Log(LogNotice, "CompatLogger")
		<< "Rescheduling rotation timer for compat log '"
		<< GetName() << "' to '" << Utility::FormatDateTime("%Y/%m/%d %H:%M:%S %z", ts) << "'";

	m_RotationTimer->Reschedule(ts);
}

/**
 * @threadsafety Always.
 */
void CompatLogger::RotationTimerHandler()
{
	try {
		ReopenFile(true);
	} catch (...) {
		ScheduleNextRotation();

		throw;
	}

	ScheduleNextRotation();
}

void CompatLogger::ValidateRotationMethod(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<CompatLogger>::ValidateRotationMethod(value, utils);

	if (value != "HOURLY" && value != "DAILY" &&
		value != "WEEKLY" && value != "MONTHLY" && value != "NONE") {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "rotation_method" }, "Rotation method '" + value + "' is invalid."));
	}
}
