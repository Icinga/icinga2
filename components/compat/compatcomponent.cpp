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

/**
 * Hint: The reason why we're using "\n" rather than std::endl is because
 * std::endl also _flushes_ the output stream which severely degrades
 * performance (see http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt11ch25s02.html).
 */

/**
 * Retrieves the status.dat path.
 *
 * @returns statuspath from config, or static default
 */
String CompatComponent::GetStatusPath(void) const
{
	Value statusPath = GetConfig()->Get("status_path");
	if (statusPath.IsEmpty())
		return Application::GetLocalStateDir() + "/cache/status.dat";
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
	Value objectsPath = GetConfig()->Get("objects_path");
	if (objectsPath.IsEmpty())
		return Application::GetLocalStateDir() + "/cache/objects.cache";
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
	Value commandPath = GetConfig()->Get("command_path");
	if (commandPath.IsEmpty())
		return Application::GetLocalStateDir() + "/run/icinga.cmd";
	else
		return commandPath;
}

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
 * Stops the component.
 */
void CompatComponent::Stop(void)
{
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
			if (unlink(commandPath.CStr()) < 0)
				throw_exception(PosixException("unlink() failed", errno));
		}
	}


	if (!fifo_ok && mkfifo(commandPath.CStr(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
		throw_exception(PosixException("mkfifo() failed", errno));

	for (;;) {
		int fd = open(commandPath.CStr(), O_RDONLY);

		if (fd < 0)
			throw_exception(PosixException("open() failed", errno));

		FILE *fp = fdopen(fd, "r");

		if (fp == NULL) {
			close(fd);
			throw_exception(PosixException("fdopen() failed", errno));
		}

		char line[2048];

		while (fgets(line, sizeof(line), fp) != NULL) {
			// remove trailing new-line
			while (strlen(line) > 0 &&
			    (line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n'))
				line[strlen(line) - 1] = '\0';

			String command = line;
			Event::Post(boost::bind(&CompatComponent::ProcessCommand, this, command));
		}

		fclose(fp);
	}
}

void CompatComponent::ProcessCommand(const String& command)
{
	try {
		Logger::Write(LogInformation, "compat", "Executing external command: " + command);

		ExternalCommandProcessor::Execute(command);
	} catch (const exception& ex) {
		stringstream msgbuf;
		msgbuf << "External command failed: " << ex.what();
		Logger::Write(LogWarning, "compat", msgbuf.str());
	}
}
#endif /* _WIN32 */

void CompatComponent::DumpComments(ofstream& fp, const DynamicObject::Ptr& owner)
{
	Service::Ptr service;
	Host::Ptr host;
	Dictionary::Ptr comments;

	if (owner->GetType() == DynamicType::GetByName("Service")) {
		service = dynamic_pointer_cast<Service>(owner);
		comments = service->GetComments();

		host = service->GetHost();
	} else {
		host = dynamic_pointer_cast<Host>(owner);
		comments = host->GetComments();
	}

	if (!comments)
		return;

	String id;
	Dictionary::Ptr comment;
	BOOST_FOREACH(tie(id, comment), comments) {
		if (CommentProcessor::IsCommentExpired(comment))
			continue;

		if (!service)
			fp << "hostcomment {" << "\n";
		else
			fp << "servicecomment {" << "\n"
			   << "\t" << "service_description=" << service->GetAlias() << "\n";

		fp << "\t" << "host_name=" << host->GetName() << "\n"
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

void CompatComponent::DumpDowntimes(ofstream& fp, const DynamicObject::Ptr& owner)
{
	Service::Ptr service;
	Host::Ptr host;
	Dictionary::Ptr downtimes;

	if (owner->GetType() == DynamicType::GetByName("Service")) {
		service = dynamic_pointer_cast<Service>(owner);
		downtimes = service->GetDowntimes();

		host = service->GetHost();
	} else {
		host = dynamic_pointer_cast<Host>(owner);
		downtimes = host->GetDowntimes();
	}

	if (!downtimes)
		return;

	String id;
	Dictionary::Ptr downtime;
	BOOST_FOREACH(tie(id, downtime), downtimes) {
		if (DowntimeProcessor::IsDowntimeExpired(downtime))
			continue;

		if (!service)
			fp << "hostdowntime {" << "\n";
		else
			fp << "servicedowntime {" << "\n"
			   << "\t" << "service_description=" << service->GetAlias() << "\n";

		Dictionary::Ptr triggeredByObj = DowntimeProcessor::GetDowntimeByID(downtime->Get("triggered_by"));
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
		   << "\t" << "is_in_effect=" << (DowntimeProcessor::IsDowntimeActive(downtime) ? 1 : 0) << "\n"
		   << "\t" << "author=" << static_cast<String>(downtime->Get("author")) << "\n"
		   << "\t" << "comment=" << static_cast<String>(downtime->Get("comment")) << "\n"
		   << "\t" << "trigger_time=" << static_cast<double>(downtime->Get("trigger_time")) << "\n"
		   << "\t" << "}" << "\n"
		   << "\n";
	}
}

void CompatComponent::DumpHostStatus(ofstream& fp, const Host::Ptr& host)
{
	int state;
	if (!host->IsReachable())
		state = 2; /* unreachable */
	else if (!host->IsUp())
		state = 1; /* down */
	else
		state = 0; /* up */

	fp << "hoststatus {" << "\n"
	   << "\t" << "host_name=" << host->GetName() << "\n"
	   << "\t" << "has_been_checked=1" << "\n"
	   << "\t" << "should_be_scheduled=1" << "\n"
	   << "\t" << "check_execution_time=0" << "\n"
	   << "\t" << "check_latency=0" << "\n"
	   << "\t" << "current_state=" << state << "\n"
	   << "\t" << "state_type=1" << "\n"
	   << "\t" << "last_check=" << Utility::GetTime() << "\n"
	   << "\t" << "next_check=" << Utility::GetTime() << "\n"
	   << "\t" << "current_attempt=1" << "\n"
	   << "\t" << "max_attempts=1" << "\n"
	   << "\t" << "active_checks_enabled=1" << "\n"
	   << "\t" << "passive_checks_enabled=1" << "\n"
	   << "\t" << "last_update=" << Utility::GetTime() << "\n"
	   << "\t" << "problem_has_been_acknowledged=" << (host->GetAcknowledgement() != AcknowledgementNone ? 1 : 0) << "\n"
	   << "\t" << "acknowledgement_type=" << static_cast<int>(host->GetAcknowledgement()) << "\n"
	   << "\t" << "acknowledgement_end_time=" << host->GetAcknowledgementExpiry() << "\n"
	   << "\t" << "scheduled_downtime_depth=" << (host->IsInDowntime() ? 1 : 0) << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";

	DumpDowntimes(fp, host);
	DumpComments(fp, host);
}

void CompatComponent::DumpHostObject(ofstream& fp, const Host::Ptr& host)
{
	fp << "define host {" << "\n"
	   << "\t" << "host_name" << "\t" << host->GetName() << "\n"
	   << "\t" << "alias" << "\t" << host->GetAlias() << "\n"
	   << "\t" << "check_interval" << "\t" << 1 << "\n"
	   << "\t" << "retry_interval" << "\t" << 1 << "\n"
	   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
	   << "\t" << "active_checks_enabled" << "\t" << 1 << "\n"
	   << "\t" << "passive_checks_enabled" << "\t" << 1 << "\n";

	set<Host::Ptr> parents = host->GetParents();

	if (!parents.empty()) {
		fp << "\t" << "parents" << "\t";
		DumpNameList(fp, parents);
		fp << "\n";
	}

	fp << "\t" << "}" << "\n"
	   << "\n";
}

void CompatComponent::DumpServiceStatus(ofstream& fp, const Service::Ptr& service)
{
	String output;
	String perfdata;
	double schedule_start = -1, schedule_end = -1;
	double execution_start = -1, execution_end = -1;

	Dictionary::Ptr cr = service->GetLastCheckResult();
	if (cr) {
		output = cr->Get("output");
		schedule_start = cr->Get("schedule_start");
		schedule_end = cr->Get("schedule_end");
		execution_start = cr->Get("execution_start");
		execution_end = cr->Get("execution_end");
		perfdata = cr->Get("performance_data_raw");
	}

	double execution_time = (execution_end - execution_start);
	double latency = (schedule_end - schedule_start) - execution_time;

	int state = service->GetState();

	if (state > StateUnknown)
		state = StateUnknown;

	fp << "servicestatus {" << "\n"
	   << "\t" << "host_name=" << service->GetHost()->GetName() << "\n"
	   << "\t" << "service_description=" << service->GetName() << "\n"
	   << "\t" << "check_interval=" << service->GetCheckInterval() / 60.0 << "\n"
	   << "\t" << "retry_interval=" << service->GetRetryInterval() / 60.0 << "\n"
	   << "\t" << "has_been_checked=" << (service->GetLastCheckResult() ? 1 : 0) << "\n"
	   << "\t" << "should_be_scheduled=1" << "\n"
	   << "\t" << "check_execution_time=" << execution_time << "\n"
	   << "\t" << "check_latency=" << latency << "\n"
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
	   << "\t" << "active_checks_enabled=" << (service->GetEnableActiveChecks() ? 1 : 0) <<"\n"
	   << "\t" << "passive_checks_enabled=" << (service->GetEnablePassiveChecks() ? 1 : 0) << "\n"
	   << "\t" << "problem_has_been_acknowledged=" << (service->GetAcknowledgement() != AcknowledgementNone ? 1 : 0) << "\n"
	   << "\t" << "acknowledgement_type=" << static_cast<int>(service->GetAcknowledgement()) << "\n"
	   << "\t" << "acknowledgement_end_time=" << service->GetAcknowledgementExpiry() << "\n"
	   << "\t" << "scheduled_downtime_depth=" << (service->IsInDowntime() ? 1 : 0) << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";

	DumpDowntimes(fp, service);
	DumpComments(fp, service);
}

void CompatComponent::DumpServiceObject(ofstream& fp, const Service::Ptr& service)
{
	fp << "define service {" << "\n"
	   << "\t" << "host_name" << "\t" << service->GetHost()->GetName() << "\n"
	   << "\t" << "service_description" << "\t" << service->GetName() << "\n"
	   << "\t" << "check_command" << "\t" << "check_i2" << "\n"
	   << "\t" << "check_interval" << "\t" << service->GetCheckInterval() / 60.0 << "\n"
	   << "\t" << "retry_interval" << "\t" << service->GetRetryInterval() / 60.0 << "\n"
	   << "\t" << "max_check_attempts" << "\t" << 1 << "\n"
	   << "\t" << "active_checks_enabled" << "\t" << (service->GetEnableActiveChecks() ? 1 : 0) << "\n"
	   << "\t" << "passive_checks_enabled" << "\t" << (service->GetEnablePassiveChecks() ? 1 : 0) << "\n"
	   << "\t" << "}" << "\n"
	   << "\n";
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
		 << "\t" << "active_host_checks_enabled=0" << "\n"
		 << "\t" << "passive_host_checks_enabled=0" << "\n"
		 << "\t" << "check_service_freshness=0" << "\n"
		 << "\t" << "check_host_freshness=0" << "\n"
		 << "\t" << "enable_flap_detection=1" << "\n"
		 << "\t" << "enable_failure_prediction=0" << "\n"
		 << "\t" << "active_scheduled_service_check_stats=" << CIB::GetActiveChecksStatistics(60) << "," << CIB::GetActiveChecksStatistics(5 * 60) << "," << CIB::GetActiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "passive_service_check_stats=" << CIB::GetPassiveChecksStatistics(60) << "," << CIB::GetPassiveChecksStatistics(5 * 60) << "," << CIB::GetPassiveChecksStatistics(15 * 60) << "\n"
		 << "\t" << "next_downtime_id=" << DowntimeProcessor::GetNextDowntimeID() << "\n"
		 << "\t" << "next_comment_id=" << CommentProcessor::GetNextCommentID() << "\n"
		 << "\t" << "}" << "\n"
		 << "\n";

	ofstream objectfp;
	objectfp.open(objectspathtmp.CStr(), ofstream::out | ofstream::trunc);

	objectfp << std::fixed;

	objectfp << "# Icinga objects cache file" << "\n"
		 << "# This file is auto-generated. Do not modify this file." << "\n"
		 << "\n";

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Host")->GetObjects()) {
		const Host::Ptr& host = static_pointer_cast<Host>(object);

		DumpHostStatus(statusfp, host);
		DumpHostObject(objectfp, host);
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("HostGroup")->GetObjects()) {
		const HostGroup::Ptr& hg = static_pointer_cast<HostGroup>(object);

		objectfp << "define hostgroup {" << "\n"
			 << "\t" << "hostgroup_name" << "\t" << hg->GetName() << "\n"
			 << "\t" << "alias" << "\t" << hg->GetAlias() << "\n"
			 << "\t" << "notes_url" << "\t" << hg->GetNotesUrl() << "\n"
			 << "\t" << "action_url" << "\t" << hg->GetActionUrl() << "\n";

		objectfp << "\t" << "members" << "\t";
		DumpNameList(objectfp, hg->GetMembers());
		objectfp << "\n"
			 << "}" << "\n";
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);

		DumpServiceStatus(statusfp, service);
		DumpServiceObject(objectfp, service);
	}

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("ServiceGroup")->GetObjects()) {
		const ServiceGroup::Ptr& sg = static_pointer_cast<ServiceGroup>(object);

		objectfp << "define servicegroup {" << "\n"
			 << "\t" << "servicegroup_name" << "\t" << sg->GetName() << "\n"
			 << "\t" << "alias" << "\t" << sg->GetAlias() << "\n"
			 << "\t" << "notes_url" << "\t" << sg->GetNotesUrl() << "\n"
			 << "\t" << "action_url" << "\t" << sg->GetActionUrl() << "\n";

		objectfp << "\t" << "members" << "\t";

		vector<String> sglist;
		BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
			sglist.push_back(service->GetHost()->GetName());
			sglist.push_back(service->GetName());
		}

		DumpStringList(objectfp, sglist);

		objectfp << "\n"
			 << "}" << "\n";
	}

	statusfp.close();
	objectfp.close();

#ifdef _WIN32
	_unlink(statuspath.CStr());
	_unlink(objectspath.CStr());
#endif /* _WIN32 */

	statusfp.close();
	if (rename(statuspathtmp.CStr(), statuspath.CStr()) < 0)
		throw_exception(PosixException("rename() failed", errno));

	objectfp.close();
	if (rename(objectspathtmp.CStr(), objectspath.CStr()) < 0)
		throw_exception(PosixException("rename() failed", errno));	
}

EXPORT_COMPONENT(compat, CompatComponent);
