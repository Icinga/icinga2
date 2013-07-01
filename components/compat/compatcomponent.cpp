/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "compat/compatcomponent.h"
#include "icinga/externalcommandprocessor.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "icinga/hostgroup.h"
#include "icinga/servicegroup.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/timeperiod.h"
#include "icinga/notificationcommand.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/application.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

using namespace icinga;

REGISTER_TYPE(CompatComponent);

/**
 * Hint: The reason why we're using "\n" rather than std::endl is because
 * std::endl also _flushes_ the output stream which severely degrades
 * performance (see http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt11ch25s02.html).
 */

/**
 * Starts the component.
 */
void CompatComponent::Start(void)
{
	m_StatusTimer = boost::make_shared<Timer>();
	m_StatusTimer->SetInterval(15);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&CompatComponent::StatusTimerHandler, this));
	m_StatusTimer->Start();
	m_StatusTimer->Reschedule(0);

#ifndef _WIN32
	m_CommandThread = boost::thread(boost::bind(&CompatComponent::CommandPipeThread, this, GetCommandPath()));
	m_CommandThread.detach();
#endif /* _WIN32 */
}

/**
 * Retrieves the status.dat path.
 *
 * @returns statuspath from config, or static default
 */
String CompatComponent::GetStatusPath(void) const
{
	Value statusPath = m_StatusPath;
	if (statusPath.IsEmpty())
		return Application::GetLocalStateDir() + "/cache/icinga2/status.dat";
	else
		return statusPath;
}

/**
 * Retrieves the objects.cache path.
 *
 * @returns objectspath from config, or static default
 */
String CompatComponent::GetObjectsPath(void) const
{
	Value objectsPath = m_ObjectsPath;
	if (objectsPath.IsEmpty())
		return Application::GetLocalStateDir() + "/cache/icinga2/objects.cache";
	else
		return objectsPath;
}

/**
 * Retrieves the icinga.cmd path.
 *
 * @returns icinga.cmd path
 */
String CompatComponent::GetCommandPath(void) const
{
	Value commandPath = m_CommandPath;
	if (commandPath.IsEmpty())
		return Application::GetLocalStateDir() + "/run/icinga2/icinga2.cmd";
	else
		return commandPath;
}

CompatComponent::CompatComponent(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("status_path", Attribute_Config, &m_StatusPath);
	RegisterAttribute("objects_path", Attribute_Config, &m_ObjectsPath);
	RegisterAttribute("command_path", Attribute_Config, &m_CommandPath);
}

#ifndef _WIN32
void CompatComponent::CommandPipeThread(const String& commandPath)
{
	struct stat statbuf;
	bool fifo_ok = false;

	if (lstat(commandPath.CStr(), &statbuf) >= 0) {
		if (S_ISFIFO(statbuf.st_mode) && access(commandPath.CStr(), R_OK) >= 0) {
			fifo_ok = true;
		} else {
			if (unlink(commandPath.CStr()) < 0) {
				BOOST_THROW_EXCEPTION(posix_error()
				    << boost::errinfo_api_function("unlink")
				    << boost::errinfo_errno(errno)
				    << boost::errinfo_file_name(commandPath));
			}
		}
	}

	if (!fifo_ok && mkfifo(commandPath.CStr(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("mkfifo")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(commandPath));
	}

	for (;;) {
		int fd;

		do {
			fd = open(commandPath.CStr(), O_RDONLY);
		} while (fd < 0 && errno == EINTR);

		if (fd < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("open")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(commandPath));
		}

		FILE *fp = fdopen(fd, "r");

		if (fp == NULL) {
			(void) close(fd);
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("fdopen")
			    << boost::errinfo_errno(errno));
		}

		char line[2048];

		while (fgets(line, sizeof(line), fp) != NULL) {
			// remove trailing new-line
			while (strlen(line) > 0 &&
			    (line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n'))
				line[strlen(line) - 1] = '\0';

			String command = line;

			try {
				Log(LogInformation, "compat", "Executing external command: " + command);

				ExternalCommandProcessor::Execute(command);
			} catch (const std::exception& ex) {
				std::ostringstream msgbuf;
				msgbuf << "External command failed: " << boost::diagnostic_information(ex);
				Log(LogWarning, "compat", msgbuf.str());
			}
		}

		fclose(fp);
	}
}
#endif /* _WIN32 */

void CompatComponent::DumpComments(std::ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Service::Ptr service;
	Host::Ptr host;
	Dictionary::Ptr comments = owner->GetComments();

	if (!comments)
		return;

	ObjectLock olock(comments);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(boost::tie(id, comment), comments) {
		if (Service::IsCommentExpired(comment))
			continue;

		if (type == CompatTypeHost)
			fp << "hostcomment {" << "\n";
		else
			fp << "servicecomment {" << "\n"
			   << "\t" << "service_description=" << owner->GetShortName() << "\n";

		fp << "\t" << "host_name=" << owner->GetHost()->GetName() << "\n"
		   << "\t" << "comment_id=" << static_cast<String>(comment->Get("legacy_id")) << "\n"
		   << "\t" << "entry_time=" << static_cast<double>(comment->Get("entry_time")) << "\n"
		   << "\t" << "entry_type=" << static_cast<long>(comment->Get("entry_type")) << "\n"
		   << "\t" << "persistent=" << 1 << "\n"
		   << "\t" << "author=" << static_cast<String>(comment->Get("author")) << "\n"
		   << "\t" << "comment_data=" << static_cast<String>(comment->Get("text")) << "\n"
		   << "\t" << "expires=" << (static_cast<double>(comment->Get("expire_time")) != 0 ? 1 : 0) << "\n"
		   << "\t" << "expire_time=" << static_cast<double>(comment->Get("expire_time")) << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void CompatComponent::DumpDowntimes(std::ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Host::Ptr host = owner->GetHost();

	if (!host)
		return;

	Dictionary::Ptr downtimes = owner->GetDowntimes();

	if (!downtimes)
		return;

	ObjectLock olock(downtimes);

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(boost::tie(id, downtime), downtimes) {
		if (Service::IsDowntimeExpired(downtime))
			continue;

		if (type == CompatTypeHost)
			fp << "hostdowntime {" << "\n";
		else
			fp << "servicedowntime {" << "\n"
			   << "\t" << "service_description=" << owner->GetShortName() << "\n";

		Dictionary::Ptr triggeredByObj = Service::GetDowntimeByID(downtime->Get("triggered_by"));
		int triggeredByLegacy = 0;
		if (triggeredByObj)
			triggeredByLegacy = triggeredByObj->Get("legacy_id");

		fp << "\t" << "host_name=" << host->GetName() << "\n"
		   << "\t" << "downtime_id=" << static_cast<String>(downtime->Get("legacy_id")) << "\n"
		   << "\t" << "entry_time=" << static_cast<double>(downtime->Get("entry_time")) << "\n"
		   << "\t" << "start_time=" << static_cast<double>(downtime->Get("start_time")) << "\n"
		   << "\t" << "end_time=" << static_cast<double>(downtime->Get("end_time")) << "\n"
		   << "\t" << "triggered_by=" << triggeredByLegacy << "\n"
		   << "\t" << "fixed=" << static_cast<long>(downtime->Get("fixed")) << "\n"
		   << "\t" << "duration=" << static_cast<long>(downtime->Get("duration")) << "\n"
		   << "\t" << "is_in_effect=" << (Service::IsDowntimeActive(downtime) ? 1 : 0) << "\n"
		   << "\t" << "author=" << static_cast<String>(downtime->Get("author")) << "\n"
		   << "\t" << "comment=" << static_cast<String>(downtime->Get("comment")) << "\n"
		   << "\t" << "trigger_time=" << static_cast<double>(downtime->Get("trigger_time")) << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void CompatComponent::DumpHostStatus(std::ostream& fp, const Host::Ptr& host)
{
	fp << "hoststatus {" << "\n"
	   << "\t" << "host_name=" << host->GetName() << "\n";

	Service::Ptr hc = host->GetHostCheckService();
	ObjectLock olock(hc);

	if (hc)
		DumpServiceStatusAttrs(fp, hc, CompatTypeHost);

	fp << "\t" << "}" << "\n"
	   << "\n";

	if (hc) {
		DumpDowntimes(fp, hc, CompatTypeHost);
		DumpComments(fp, hc, CompatTypeHost);
	}
}

void CompatComponent::DumpHostObject(std::ostream& fp, const Host::Ptr& host)
{
	fp << "define host {" << "\n"
	   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
	   << "\t" << "display_name" << "\t" << host->GetDisplayName() << "\n";

	std::set<Host::Ptr> parents = host->GetParentHosts();

	if (!parents.empty()) {
		fp << "\t" << "parents" << "\t";
		DumpNameList(fp, parents);
		fp << "\n";
	}

	Service::Ptr hc = host->GetHostCheckService();
	if (hc) {
		ObjectLock olock(hc);

		fp << "\t" << "check_interval" << "\t" << hc->GetCheckInterval() / 60.0 << "\n"
		   << "\t" << "retry_interval" << "\t" << hc->GetRetryInterval() / 60.0 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << hc->GetMaxCheckAttempts() << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << (hc->GetEnableActiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << (hc->GetEnablePassiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "notifications_enabled" << "\t" << (hc->GetEnableNotifications() ? 1 : 0) << "\n"
		   << "\t" << "notification_options" << "\t" << "d,u,r" << "\n"
		   << "\t" << "notification_interval" << "\t" << 1 << "\n";
	} else {
		fp << "\t" << "check_interval" << "\t" << 60 << "\n"
		   << "\t" << "retry_interval" << "\t" << 60 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << 0 << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << 0 << "\n"
		   << "\t" << "notifications_enabled" << "\t" << 0 << "\n";

	}

	DumpCustomAttributes(fp, host);

	fp << "\t" << "}" << "\n"
	   << "\n";
}

void CompatComponent::DumpServiceStatusAttrs(std::ostream& fp, const Service::Ptr& service, CompatObjectType type)
{
	ASSERT(service->OwnsLock());

	String raw_output;
	String output;
	String long_output;
	String perfdata;
	String check_command = "";
	String event_command = "";
	double schedule_end = -1;

	CheckCommand::Ptr checkcommand = service->GetCheckCommand();
	if (checkcommand)
		check_command = checkcommand->GetName();

	EventCommand::Ptr eventcommand = service->GetEventCommand();
	if (eventcommand)
		event_command = eventcommand->GetName();

	String check_period_str;
	TimePeriod::Ptr check_period = service->GetCheckPeriod();
	if (check_period)
		check_period_str = check_period->GetName();
	else
		check_period_str = "24x7";

	Dictionary::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		raw_output = cr->Get("output");
		size_t line_end = raw_output.Find("\n");

		output = raw_output.SubStr(0, line_end);

		if (line_end > 0 && line_end != String::NPos) {
			long_output = raw_output.SubStr(line_end+1, raw_output.GetLength());
			boost::algorithm::replace_all(long_output, "\n", "\\n");
		}

		boost::algorithm::replace_all(output, "\n", "\\n");

		schedule_end = cr->Get("schedule_end");

		perfdata = cr->Get("performance_data_raw");
		boost::algorithm::replace_all(perfdata, "\n", "\\n");
	}

	int state = service->GetState();

	if (state > StateUnknown)
		state = StateUnknown;

	if (type == CompatTypeHost) {
		if (state == StateOK || state == StateWarning)
			state = 0; /* UP */
		else
			state = 1; /* DOWN */

		Host::Ptr host = service->GetHost();

		if (!host)
			return;

		if (!host->IsReachable())
			state = 2; /* UNREACHABLE */
	}

	double last_notification = 0;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification->GetLastNotification() > last_notification)
			last_notification = notification->GetLastNotification();
	}

	fp << "\t" << "check_command=" << check_command << "\n"
	   << "\t" << "check_period=" << check_period_str << "\n"
	   << "\t" << "event_handler=" << event_command << "\n"
	   << "\t" << "check_interval=" << service->GetCheckInterval() / 60.0 << "\n"
	   << "\t" << "retry_interval=" << service->GetRetryInterval() / 60.0 << "\n"
	   << "\t" << "has_been_checked=" << (service->GetLastCheckResult() ? 1 : 0) << "\n"
	   << "\t" << "should_be_scheduled=1" << "\n"
	   << "\t" << "check_execution_time=" << Service::CalculateExecutionTime(cr) << "\n"
	   << "\t" << "check_latency=" << Service::CalculateLatency(cr) << "\n"
	   << "\t" << "current_state=" << state << "\n"
	   << "\t" << "state_type=" << service->GetStateType() << "\n"
	   << "\t" << "plugin_output=" << output << "\n"
	   << "\t" << "long_plugin_output=" << long_output << "\n"
	   << "\t" << "performance_data=" << perfdata << "\n"
	   << "\t" << "last_check=" << schedule_end << "\n"
	   << "\t" << "next_check=" << service->GetNextCheck() << "\n"
	   << "\t" << "current_attempt=" << service->GetCurrentCheckAttempt() << "\n"
	   << "\t" << "max_attempts=" << service->GetMaxCheckAttempts() << "\n"
	   << "\t" << "last_state_change=" << service->GetLastStateChange() << "\n"
	   << "\t" << "last_hard_state_change=" << service->GetLastHardStateChange() << "\n"
	   << "\t" << "last_update=" << time(NULL) << "\n"
	   << "\t" << "notifications_enabled=" << (service->GetEnableNotifications() ? 1 : 0) << "\n"
	   << "\t" << "active_checks_enabled=" << (service->GetEnableActiveChecks() ? 1 : 0) <<"\n"
	   << "\t" << "passive_checks_enabled=" << (service->GetEnablePassiveChecks() ? 1 : 0) << "\n"
	   << "\t" << "flap_detection_enabled=" << "\t" << (service->GetEnableFlapping() ? 1 : 0) << "\n"
	   << "\t" << "is_flapping=" << "\t" << (service->IsFlapping() ? 1 : 0) << "\n"
	   << "\t" << "percent_state_change=" << "\t" << Convert::ToString(service->GetFlappingCurrent()) << "\n"
	   << "\t" << "problem_has_been_acknowledged=" << (service->GetAcknowledgement() != AcknowledgementNone ? 1 : 0) << "\n"
	   << "\t" << "acknowledgement_type=" << static_cast<int>(service->GetAcknowledgement()) << "\n"
	   << "\t" << "acknowledgement_end_time=" << service->GetAcknowledgementExpiry() << "\n"
	   << "\t" << "scheduled_downtime_depth=" << (service->IsInDowntime() ? 1 : 0) << "\n"
	   << "\t" << "last_notification=" << last_notification << "\n";
}

void CompatComponent::DumpServiceStatus(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	fp << "servicestatus {" << "\n"
	   << "\t" << "host_name=" << host->GetName() << "\n"
	   << "\t" << "service_description=" << service->GetShortName() << "\n";

	{
		ObjectLock olock(service);
		DumpServiceStatusAttrs(fp, service, CompatTypeService);
	}

	fp << "\t" << "}" << "\n"
	   << "\n";

	DumpDowntimes(fp, service, CompatTypeService);
	DumpComments(fp, service, CompatTypeService);
}

void CompatComponent::DumpServiceObject(std::ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	String check_command = "";

        CheckCommand::Ptr checkcommand = service->GetCheckCommand();
        if (checkcommand)
                check_command = checkcommand->GetName();

        String check_period_str;
        TimePeriod::Ptr check_period = service->GetCheckPeriod();
        if (check_period)
                check_period_str = check_period->GetName();
        else
                check_period_str = "24x7";

	double notification_interval = -1;
	BOOST_FOREACH(const Notification::Ptr& notification, service->GetNotifications()) {
		if (notification_interval == -1 || notification->GetNotificationInterval() < notification_interval)
			notification_interval = notification->GetNotificationInterval();
	}

	if (notification_interval == -1)
		notification_interval = 60;

	{
		ObjectLock olock(service);

		fp << "define service {" << "\n"
		   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
		   << "\t" << "service_description" << "\t" << service->GetShortName() << "\n"
		   << "\t" << "display_name" << "\t" << service->GetDisplayName() << "\n"
		   << "\t" << "check_period" << "\t" << check_period_str << "\n"
		   << "\t" << "check_command" << "\t" << check_command << "\n"
		   << "\t" << "check_interval" << "\t" << service->GetCheckInterval() / 60.0 << "\n"
		   << "\t" << "retry_interval" << "\t" << service->GetRetryInterval() / 60.0 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << service->GetMaxCheckAttempts() << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << (service->GetEnableActiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << (service->GetEnablePassiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "flap_detection_enabled" << "\t" << (service->GetEnableFlapping() ? 1 : 0) << "\n"
		   << "\t" << "is_volatile" << "\t" << (service->IsVolatile() ? 1 : 0) << "\n"
		   << "\t" << "notifications_enabled" << "\t" << (service->GetEnableNotifications() ? 1 : 0) << "\n"
		   << "\t" << "notification_options" << "\t" << "u,w,c,r" << "\n"
   		   << "\t" << "notification_interval" << "\t" << notification_interval / 60.0 << "\n";
	}

	DumpCustomAttributes(fp, service);

	fp << "\t" << "}" << "\n"
	   << "\n";

	BOOST_FOREACH(const Service::Ptr& parent, service->GetParentServices()) {
		Host::Ptr host = service->GetHost();

		if (!host)
			continue;

		Host::Ptr parent_host = parent->GetHost();

		if (!parent_host)
			continue;

		fp << "define servicedependency {" << "\n"
		   << "\t" << "dependent_host_name" << "\t" << host->GetName() << "\n"
		   << "\t" << "dependent_service_description" << "\t" << service->GetShortName() << "\n"
		   << "\t" << "host_name" << "\t" << parent_host->GetName() << "\n"
		   << "\t" << "service_description" << "\t" << service->GetShortName() << "\n"
		   << "\t" << "execution_failure_criteria" << "\t" << "n" << "\n"
		   << "\t" << "notification_failure_criteria" << "\t" << "w,u,c" << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void CompatComponent::DumpCustomAttributes(std::ostream& fp, const DynamicObject::Ptr& object)
{
	Dictionary::Ptr custom = object->Get("custom");

	if (!custom)
		return;

	ObjectLock olock(custom);
	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), custom) {
		fp << "\t";

		if (key != "notes" && key != "action_url" && key != "notes_url" &&
		    key != "icon_image" && key != "icon_image_alt" && key != "statusmap_image" && "2d_coords")
			fp << "_";

		fp << key << "\t" << Convert::ToString(value) << "\n";
	}
}

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void CompatComponent::StatusTimerHandler(void)
{
	Log(LogInformation, "compat", "Writing compat status information");

	String statuspath = GetStatusPath();
	String objectspath = GetObjectsPath();
	String statuspathtmp = statuspath + ".tmp"; /* XXX make this a global definition */
	String objectspathtmp = objectspath + ".tmp";

	std::ofstream statusfp;
	statusfp.open(statuspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	statusfp << std::fixed;

	statusfp << "# Icinga status file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	statusfp << "info {" << "\n"
		 << "\t" << "created=" << Utility::GetTime() << "\n"
		 << "\t" << "version=2.0" << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	statusfp << "programstatus {" << "\n"
		 << "icinga_pid=" << Utility::GetPid() << "\n"
		 << "\t" << "daemon_mode=1" << "\n"
		 << "\t" << "program_start=" << IcingaApplication::GetInstance()->GetStartTime() << "\n"
		 << "\t" << "active_service_checks_enabled=1" << "\n"
		 << "\t" << "passive_service_checks_enabled=1" << "\n"
		 << "\t" << "active_host_checks_enabled=1" << "\n"
		 << "\t" << "passive_host_checks_enabled=1" << "\n"
		 << "\t" << "check_service_freshness=0" << "\n"
		 << "\t" << "check_host_freshness=0" << "\n"
		 << "\t" << "enable_notifications=1" << "\n"
		 << "\t" << "enable_flap_detection=1" << "\n"
		 << "\t" << "enable_failure_prediction=0" << "\n"
		 << "\t" << "active_scheduled_service_check_stats=" << CIB::GetActiveChecksStatistics(60) << "," << CIB::GetActiveChecksStatistics(5 * 60) << "," << CIB::GetActiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "passive_service_check_stats=" << CIB::GetPassiveChecksStatistics(60) << "," << CIB::GetPassiveChecksStatistics(5 * 60) << "," << CIB::GetPassiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "next_downtime_id=" << Service::GetNextDowntimeID() << "\n"
		 << "\t" << "next_comment_id=" << Service::GetNextCommentID() << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	std::ofstream objectfp;
	objectfp.open(objectspathtmp.CStr(), std::ofstream::out | std::ofstream::trunc);

	objectfp << std::fixed;

	objectfp << "# Icinga objects cache file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		Host::Ptr host = static_pointer_cast<Host>(object);

		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpHostStatus(tempstatusfp, host);
		statusfp << tempstatusfp.str();

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpHostObject(tempobjectfp, host);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("HostGroup")) {
		HostGroup::Ptr hg = static_pointer_cast<HostGroup>(object);

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define hostgroup {" << "\n"
			 << "\t" << "hostgroup_name" << "\t" << hg->GetName() << "\n";

		DumpCustomAttributes(tempobjectfp, hg);

		tempobjectfp << "\t" << "members" << "\t";
		DumpNameList(tempobjectfp, hg->GetMembers());
		tempobjectfp << "\n"
			     << "\t" << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		std::ostringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpServiceStatus(tempstatusfp, service);
		statusfp << tempstatusfp.str();

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpServiceObject(tempobjectfp, service);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("ServiceGroup")) {
		ServiceGroup::Ptr sg = static_pointer_cast<ServiceGroup>(object);

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define servicegroup {" << "\n"
			 << "\t" << "servicegroup_name" << "\t" << sg->GetName() << "\n";

		DumpCustomAttributes(tempobjectfp, sg);

		tempobjectfp << "\t" << "members" << "\t";

		std::vector<String> sglist;
		BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
			Host::Ptr host = service->GetHost();

			if (!host)
				continue;

			sglist.push_back(host->GetName());
			sglist.push_back(service->GetShortName());
		}

		DumpStringList(tempobjectfp, sglist);

		tempobjectfp << "\n"
			 << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("User")) {
		User::Ptr user = static_pointer_cast<User>(object);

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define contact {" << "\n"
			 << "\t" << "contact_name" << "\t" << user->GetName() << "\n"
			 << "\t" << "alias" << "\t" << user->GetDisplayName() << "\n"
			 << "\t" << "service_notification_options" << "\t" << "w,u,c,r,f,s" << "\n"
			 << "\t" << "host_notification_options" << "\t" << "d,u,r,f,s" << "\n"
			 << "\t" << "host_notifications_enabled" << "\t" << 1 << "\n"
			 << "\t" << "service_notifications_enabled" << "\t" << 1 << "\n"
			 << "\t" << "}" << "\n"
			 << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("UserGroup")) {
		UserGroup::Ptr ug = static_pointer_cast<UserGroup>(object);

		std::ostringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define contactgroup {" << "\n"
			 << "\t" << "contactgroup_name" << "\t" << ug->GetName() << "\n"
			 << "\t" << "alias" << "\t" << ug->GetDisplayName() << "\n";

		tempobjectfp << "\t" << "members" << "\t";
		DumpNameList(tempobjectfp, ug->GetMembers());
		tempobjectfp << "\n"
			     << "\t" << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	statusfp.close();
	objectfp.close();

#ifdef _WIN32
	_unlink(statuspath.CStr());
	_unlink(objectspath.CStr());
#endif /* _WIN32 */

	statusfp.close();
	if (rename(statuspathtmp.CStr(), statuspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(statuspathtmp));
	}

	objectfp.close();
	if (rename(objectspathtmp.CStr(), objectspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(objectspathtmp));
	}
}
