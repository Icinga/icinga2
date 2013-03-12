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

#include "i2-compat.h"

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
	m_CommandThread = thread(boost::bind(&CompatComponent::CommandPipeThread, this, GetCommandPath()));
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
 * Retrieves the log path.
 *
 * @returns log path
 */
String CompatComponent::GetLogPath(void) const
{
	Value logPath = m_LogPath;
	if (logPath.IsEmpty())
		return Application::GetLocalStateDir() + "/log/icinga2/compat";
	else
		return logPath;
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
	RegisterAttribute("log_path", Attribute_Config, &m_LogPath);
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
				    << errinfo_api_function("unlink")
				    << errinfo_errno(errno)
				    << errinfo_file_name(commandPath));
			}
		}
	}

	if (!fifo_ok && mkfifo(commandPath.CStr(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << errinfo_api_function("mkfifo")
		    << errinfo_errno(errno)
		    << errinfo_file_name(commandPath));
	}

	for (;;) {
		int fd;

		do {
			fd = open(commandPath.CStr(), O_RDONLY);
		} while (fd < 0 && errno == EINTR);

		if (fd < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << errinfo_api_function("open")
			    << errinfo_errno(errno)
			    << errinfo_file_name(commandPath));
		}

		FILE *fp = fdopen(fd, "r");

		if (fp == NULL) {
			(void) close(fd);
			BOOST_THROW_EXCEPTION(posix_error()
			    << errinfo_api_function("fdopen")
			    << errinfo_errno(errno));
		}

		char line[2048];

		while (fgets(line, sizeof(line), fp) != NULL) {
			// remove trailing new-line
			while (strlen(line) > 0 &&
			    (line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n'))
				line[strlen(line) - 1] = '\0';

			String command = line;

			try {
				Logger::Write(LogInformation, "compat", "Executing external command: " + command);

				ExternalCommandProcessor::Execute(command);
			} catch (const exception& ex) {
				stringstream msgbuf;
				msgbuf << "External command failed: " << diagnostic_information(ex);
				Logger::Write(LogWarning, "compat", msgbuf.str());
			}
		}

		fclose(fp);
	}
}
#endif /* _WIN32 */

void CompatComponent::DumpComments(ostream& fp, const Service::Ptr& owner, CompatObjectType type)
{
	Service::Ptr service;
	Host::Ptr host;
	Dictionary::Ptr comments = owner->GetComments();

	if (!comments)
		return;

	ObjectLock olock(comments);

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(tie(id, comment), comments) {
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

void CompatComponent::DumpDowntimes(ostream& fp, const Service::Ptr& owner, CompatObjectType type)
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
	BOOST_FOREACH(tie(id, downtime), downtimes) {
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

void CompatComponent::DumpHostStatus(ostream& fp, const Host::Ptr& host)
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

void CompatComponent::DumpHostObject(ostream& fp, const Host::Ptr& host)
{
	fp << "define host {" << "\n"
	   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
	   << "\t" << "display_name" << "\t" << host->GetDisplayName() << "\n";

	set<Host::Ptr> parents = host->GetParentHosts();

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
		   << "\t" << "notification_interval" << "\t" << hc->GetNotificationInterval() << "\n";
	} else {
		fp << "\t" << "check_interval" << "\t" << 60 << "\n"
		   << "\t" << "retry_interval" << "\t" << 60 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << 0 << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << 0 << "\n"
		   << "\t" << "notifications_enabled" << "\t" << 0 << "\n";

	}

	fp << "\t" << "}" << "\n"
	   << "\n";
}

void CompatComponent::DumpServiceStatusAttrs(ostream& fp, const Service::Ptr& service, CompatObjectType type)
{
	ASSERT(service->OwnsLock());

	String output;
	String perfdata;
	double schedule_end = -1;

	Dictionary::Ptr cr = service->GetLastCheckResult();

	if (cr) {
		output = cr->Get("output");
		schedule_end = cr->Get("schedule_end");
		perfdata = cr->Get("performance_data_raw");
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

	fp << "\t" << "check_interval=" << service->GetCheckInterval() / 60.0 << "\n"
	   << "\t" << "retry_interval=" << service->GetRetryInterval() / 60.0 << "\n"
	   << "\t" << "has_been_checked=" << (service->GetLastCheckResult() ? 1 : 0) << "\n"
	   << "\t" << "should_be_scheduled=1" << "\n"
	   << "\t" << "check_execution_time=" << Service::CalculateExecutionTime(cr) << "\n"
	   << "\t" << "check_latency=" << Service::CalculateLatency(cr) << "\n"
	   << "\t" << "current_state=" << state << "\n"
	   << "\t" << "state_type=" << service->GetStateType() << "\n"
	   << "\t" << "plugin_output=" << output << "\n"
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
	   << "\t" << "problem_has_been_acknowledged=" << (service->GetAcknowledgement() != AcknowledgementNone ? 1 : 0) << "\n"
	   << "\t" << "acknowledgement_type=" << static_cast<int>(service->GetAcknowledgement()) << "\n"
	   << "\t" << "acknowledgement_end_time=" << service->GetAcknowledgementExpiry() << "\n"
	   << "\t" << "scheduled_downtime_depth=" << (service->IsInDowntime() ? 1 : 0) << "\n"
	   << "\t" << "last_notification=" << service->GetLastNotification() << "\n";
}

void CompatComponent::DumpServiceStatus(ostream& fp, const Service::Ptr& service)
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

void CompatComponent::DumpServiceObject(ostream& fp, const Service::Ptr& service)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	{
		ObjectLock olock(service);

		fp << "define service {" << "\n"
		   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
		   << "\t" << "service_description" << "\t" << service->GetShortName() << "\n"
		   << "\t" << "display_name" << "\t" << service->GetDisplayName() << "\n"
		   << "\t" << "check_command" << "\t" << "check_i2" << "\n"
		   << "\t" << "check_interval" << "\t" << service->GetCheckInterval() / 60.0 << "\n"
		   << "\t" << "retry_interval" << "\t" << service->GetRetryInterval() / 60.0 << "\n"
		   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
		   << "\t" << "active_checks_enabled" << "\t" << (service->GetEnableActiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "passive_checks_enabled" << "\t" << (service->GetEnablePassiveChecks() ? 1 : 0) << "\n"
		   << "\t" << "notifications_enabled" << "\t" << (service->GetEnableNotifications() ? 1 : 0) << "\n"
		   << "\t" << "notification_options" << "\t" << "u,w,c,r" << "\n"
   		   << "\t" << "notification_interval" << "\t" << service->GetNotificationInterval() << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}

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

/**
 * Periodically writes the status.dat and objects.cache files.
 */
void CompatComponent::StatusTimerHandler(void)
{
	Logger::Write(LogInformation, "compat", "Writing compat status information");

	String statuspath = GetStatusPath();
	String objectspath = GetObjectsPath();
	String statuspathtmp = statuspath + ".tmp"; /* XXX make this a global definition */
	String objectspathtmp = objectspath + ".tmp";

	ofstream statusfp;
	statusfp.open(statuspathtmp.CStr(), ofstream::out | ofstream::trunc);

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
		 << "\t" << "enable_flap_detection=0" << "\n"
		 << "\t" << "enable_failure_prediction=0" << "\n"
		 << "\t" << "active_scheduled_service_check_stats=" << CIB::GetActiveChecksStatistics(60) << "," << CIB::GetActiveChecksStatistics(5 * 60) << "," << CIB::GetActiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "passive_service_check_stats=" << CIB::GetPassiveChecksStatistics(60) << "," << CIB::GetPassiveChecksStatistics(5 * 60) << "," << CIB::GetPassiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "next_downtime_id=" << Service::GetNextDowntimeID() << "\n"
		 << "\t" << "next_comment_id=" << Service::GetNextCommentID() << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	ofstream objectfp;
	objectfp.open(objectspathtmp.CStr(), ofstream::out | ofstream::trunc);

	objectfp << std::fixed;

	objectfp << "# Icinga objects cache file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		Host::Ptr host = static_pointer_cast<Host>(object);

		stringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpHostStatus(tempstatusfp, host);
		statusfp << tempstatusfp.str();

		stringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpHostObject(tempobjectfp, host);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("HostGroup")) {
		HostGroup::Ptr hg = static_pointer_cast<HostGroup>(object);

		stringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define hostgroup {" << "\n"
			 << "\t" << "hostgroup_name" << "\t" << hg->GetName() << "\n"
			 << "\t" << "notes_url" << "\t" << hg->GetNotesUrl() << "\n"
			 << "\t" << "action_url" << "\t" << hg->GetActionUrl() << "\n";

		tempobjectfp << "\t" << "members" << "\t";
		DumpNameList(tempobjectfp, hg->GetMembers());
		tempobjectfp << "\n"
			     << "\t" << "}" << "\n";

		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		stringstream tempstatusfp;
		tempstatusfp << std::fixed;
		DumpServiceStatus(tempstatusfp, service);
		statusfp << tempstatusfp.str();

		stringstream tempobjectfp;
		tempobjectfp << std::fixed;
		DumpServiceObject(tempobjectfp, service);
		objectfp << tempobjectfp.str();
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("ServiceGroup")) {
		ServiceGroup::Ptr sg = static_pointer_cast<ServiceGroup>(object);

		stringstream tempobjectfp;
		tempobjectfp << std::fixed;

		tempobjectfp << "define servicegroup {" << "\n"
			 << "\t" << "servicegroup_name" << "\t" << sg->GetName() << "\n"
			 << "\t" << "notes_url" << "\t" << sg->GetNotesUrl() << "\n"
			 << "\t" << "action_url" << "\t" << sg->GetActionUrl() << "\n";

		tempobjectfp << "\t" << "members" << "\t";

		vector<String> sglist;
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

		stringstream tempobjectfp;
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

		stringstream tempobjectfp;
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
		    << errinfo_api_function("rename")
		    << errinfo_errno(errno)
		    << errinfo_file_name(statuspathtmp));
	}

	objectfp.close();
	if (rename(objectspathtmp.CStr(), objectspath.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << errinfo_api_function("rename")
		    << errinfo_errno(errno)
		    << errinfo_file_name(objectspathtmp));
	}
}
