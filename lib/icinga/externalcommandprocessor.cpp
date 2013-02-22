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

#include "i2-icinga.h"

using namespace icinga;

boost::once_flag ExternalCommandProcessor::m_InitializeOnce = BOOST_ONCE_INIT;
boost::mutex ExternalCommandProcessor::m_Mutex;
map<String, ExternalCommandProcessor::Callback> ExternalCommandProcessor::m_Commands;

/**
 * @threadsafety Always.
 */
void ExternalCommandProcessor::Execute(const String& line)
{
	if (line.IsEmpty())
		return;

	if (line[0] != '[')
		BOOST_THROW_EXCEPTION(invalid_argument("Missing timestamp in command: " + line));

	size_t pos = line.FindFirstOf("]");

	if (pos == String::NPos)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing timestamp in command: " + line));

	String timestamp = line.SubStr(1, pos - 1);
	String args = line.SubStr(pos + 2, String::NPos);

	double ts = Convert::ToDouble(timestamp);

	if (ts == 0)
		BOOST_THROW_EXCEPTION(invalid_argument("Invalid timestamp in command: " + line));

	vector<String> argv = args.Split(is_any_of(";"));

	if (argv.size() == 0)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing arguments in command: " + line));

	vector<String> argvExtra(argv.begin() + 1, argv.end());
	Execute(ts, argv[0], argvExtra);
}

/**
 * @threadsafety Always.
 */
void ExternalCommandProcessor::Execute(double time, const String& command, const vector<String>& arguments)
{
	boost::call_once(m_InitializeOnce, &ExternalCommandProcessor::Initialize);

	Callback callback;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		map<String, ExternalCommandProcessor::Callback>::iterator it;
		it = m_Commands.find(command);

		if (it == m_Commands.end())
			BOOST_THROW_EXCEPTION(invalid_argument("The external command '" + command + "' does not exist."));

		callback = it->second;
	}

	callback(time, arguments);
}

/**
 * @threadsafety Always.
 */
void ExternalCommandProcessor::Initialize(void)
{
	RegisterCommand("PROCESS_SERVICE_CHECK_RESULT", &ExternalCommandProcessor::ProcessServiceCheckResult);
	RegisterCommand("SCHEDULE_SVC_CHECK", &ExternalCommandProcessor::ScheduleSvcCheck);
	RegisterCommand("SCHEDULE_FORCED_SVC_CHECK", &ExternalCommandProcessor::ScheduleForcedSvcCheck);
	RegisterCommand("ENABLE_SVC_CHECK", &ExternalCommandProcessor::EnableSvcCheck);
	RegisterCommand("DISABLE_SVC_CHECK", &ExternalCommandProcessor::DisableSvcCheck);
	RegisterCommand("SHUTDOWN_PROCESS", &ExternalCommandProcessor::ShutdownProcess);
	RegisterCommand("SCHEDULE_FORCED_HOST_SVC_CHECKS", &ExternalCommandProcessor::ScheduleForcedHostSvcChecks);
	RegisterCommand("SCHEDULE_HOST_SVC_CHECKS", &ExternalCommandProcessor::ScheduleHostSvcChecks);
	RegisterCommand("ENABLE_HOST_SVC_CHECKS", &ExternalCommandProcessor::EnableHostSvcChecks);
	RegisterCommand("DISABLE_HOST_SVC_CHECKS", &ExternalCommandProcessor::DisableHostSvcChecks);
	RegisterCommand("ACKNOWLEDGE_SVC_PROBLEM", &ExternalCommandProcessor::AcknowledgeSvcProblem);
	RegisterCommand("ACKNOWLEDGE_SVC_PROBLEM_EXPIRE", &ExternalCommandProcessor::AcknowledgeSvcProblemExpire);
	RegisterCommand("REMOVE_SVC_ACKNOWLEDGEMENT", &ExternalCommandProcessor::RemoveHostAcknowledgement);
	RegisterCommand("ACKNOWLEDGE_HOST_PROBLEM", &ExternalCommandProcessor::AcknowledgeHostProblem);
	RegisterCommand("ACKNOWLEDGE_HOST_PROBLEM_EXPIRE", &ExternalCommandProcessor::AcknowledgeHostProblemExpire);
	RegisterCommand("REMOVE_HOST_ACKNOWLEDGEMENT", &ExternalCommandProcessor::RemoveHostAcknowledgement);
	RegisterCommand("ENABLE_HOSTGROUP_SVC_CHECKS", &ExternalCommandProcessor::EnableHostgroupSvcChecks);
	RegisterCommand("DISABLE_HOSTGROUP_SVC_CHECKS", &ExternalCommandProcessor::DisableHostgroupSvcChecks);
	RegisterCommand("ENABLE_SERVICEGROUP_SVC_CHECKS", &ExternalCommandProcessor::EnableServicegroupSvcChecks);
	RegisterCommand("DISABLE_SERVICEGROUP_SVC_CHECKS", &ExternalCommandProcessor::DisableServicegroupSvcChecks);
	RegisterCommand("ENABLE_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::EnablePassiveSvcChecks);
	RegisterCommand("DISABLE_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::DisablePassiveSvcChecks);
	RegisterCommand("ENABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::EnableServicegroupPassiveSvcChecks);
	RegisterCommand("DISABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::DisableServicegroupPassiveSvcChecks);
	RegisterCommand("ENABLE_HOSTGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::EnableHostgroupPassiveSvcChecks);
	RegisterCommand("DISABLE_HOSTGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::DisableHostgroupPassiveSvcChecks);
	RegisterCommand("PROCESS_FILE", &ExternalCommandProcessor::ProcessFile);
	RegisterCommand("SCHEDULE_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleSvcDowntime);
	RegisterCommand("DEL_SVC_DOWNTIME", &ExternalCommandProcessor::DelSvcDowntime);
	RegisterCommand("SCHEDULE_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleHostDowntime);
	RegisterCommand("DEL_HOST_DOWNTIME", &ExternalCommandProcessor::DelHostDowntime);
	RegisterCommand("SCHEDULE_HOST_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleHostSvcDowntime);
	RegisterCommand("SCHEDULE_HOSTGROUP_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleHostgroupHostDowntime);
	RegisterCommand("SCHEDULE_HOSTGROUP_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleHostgroupSvcDowntime);
	RegisterCommand("SCHEDULE_SERVICEGROUP_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleServicegroupHostDowntime);
	RegisterCommand("SCHEDULE_SERVICEGROUP_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleServicegroupSvcDowntime);
	RegisterCommand("ADD_HOST_COMMENT", &ExternalCommandProcessor::AddHostComment);
	RegisterCommand("DEL_HOST_COMMENT", &ExternalCommandProcessor::DelHostComment);
	RegisterCommand("ADD_SVC_COMMENT", &ExternalCommandProcessor::AddSvcComment);
	RegisterCommand("DEL_SVC_COMMENT", &ExternalCommandProcessor::DelSvcComment);
	RegisterCommand("DEL_ALL_HOST_COMMENTS", &ExternalCommandProcessor::DelAllHostComments);
	RegisterCommand("DEL_ALL_SVC_COMMENTS", &ExternalCommandProcessor::DelAllSvcComments);
	RegisterCommand("SEND_CUSTOM_HOST_NOTIFICATION", &ExternalCommandProcessor::SendCustomHostNotification);
	RegisterCommand("SEND_CUSTOM_SVC_NOTIFICATION", &ExternalCommandProcessor::SendCustomSvcNotification);
}

/**
 * @threadsafety Always.
 */
void ExternalCommandProcessor::RegisterCommand(const String& command, const ExternalCommandProcessor::Callback& callback)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Commands[command] = callback;
}

void ExternalCommandProcessor::ProcessServiceCheckResult(double time, const vector<String>& arguments)
{
	if (arguments.size() < 4)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 4 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service->GetEnablePassiveChecks())
		BOOST_THROW_EXCEPTION(invalid_argument("Got passive check result for service '" + arguments[1] + "' which has passive checks disabled."));

	int exitStatus = Convert::ToDouble(arguments[2]);
	Dictionary::Ptr result = PluginCheckTask::ParseCheckOutput(arguments[3]);
	result->Set("state", PluginCheckTask::ExitStatusToState(exitStatus));

	result->Set("schedule_start", time);
	result->Set("schedule_end", time);
	result->Set("execution_start", time);
	result->Set("execution_end", time);
	result->Set("active", 0);

	Logger::Write(LogInformation, "icinga", "Processing passive check result for service '" + arguments[1] + "'");
	service->ProcessCheckResult(result);

	/* Reschedule the next check. The side effect of this is that for as long
	 * as we receive passive results for a service we won't execute any
	 * active checks. */
	service->SetNextCheck(Utility::GetTime() + service->GetCheckInterval());
}

void ExternalCommandProcessor::ScheduleSvcCheck(double, const vector<String>& arguments)
{
	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 3 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	double planned_check = Convert::ToDouble(arguments[2]);

	if (planned_check > service->GetNextCheck()) {
		Logger::Write(LogInformation, "icinga", "Ignoring reschedule request for service '" +
		    arguments[1] + "' (next check is already sooner than requested check time)");
		return;
	}

	Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + arguments[1] + "'");
	service->SetNextCheck(planned_check);
}

void ExternalCommandProcessor::ScheduleForcedSvcCheck(double, const vector<String>& arguments)
{
	if (arguments.size() < 3)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 3 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + arguments[1] + "'");
	service->SetForceNextCheck(true);
	service->SetNextCheck(Convert::ToDouble(arguments[2]));
}

void ExternalCommandProcessor::EnableSvcCheck(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + arguments[1] + "'");
	service->SetEnableActiveChecks(true);
}

void ExternalCommandProcessor::DisableSvcCheck(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + arguments[1] + "'");
	service->SetEnableActiveChecks(false);
}

void ExternalCommandProcessor::ShutdownProcess(double, const vector<String>&)
{
	Logger::Write(LogInformation, "icinga", "Shutting down Icinga via external command.");
	Application::RequestShutdown();
}

void ExternalCommandProcessor::ScheduleForcedHostSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	double planned_check = Convert::ToDouble(arguments[1]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + service->GetName() + "'");
		service->SetNextCheck(planned_check);
		service->SetForceNextCheck(true);
	}
}

void ExternalCommandProcessor::ScheduleHostSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	double planned_check = Convert::ToDouble(arguments[1]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		if (planned_check > service->GetNextCheck()) {
			Logger::Write(LogInformation, "icinga", "Ignoring reschedule request for service '" +
			    service->GetName() + "' (next check is already sooner than requested check time)");
			continue;
		}

		Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + service->GetName() + "'");
		service->SetNextCheck(planned_check);
	}
}

void ExternalCommandProcessor::EnableHostSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(true);
	}
}

void ExternalCommandProcessor::DisableHostSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 arguments."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(false);
	}
}

void ExternalCommandProcessor::AcknowledgeSvcProblem(double, const vector<String>& arguments)
{
	if (arguments.size() < 7)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 7 arguments."));

	bool sticky = Convert::ToBool(arguments[2]);

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (service->GetState() == StateOK)
		BOOST_THROW_EXCEPTION(invalid_argument("The service '" + arguments[1] + "' is OK."));

	Logger::Write(LogInformation, "icinga", "Setting acknowledgement for service '" + service->GetName() + "'");
	service->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
	service->SetAcknowledgementExpiry(0);
}

void ExternalCommandProcessor::AcknowledgeSvcProblemExpire(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 arguments."));

	bool sticky = Convert::ToBool(arguments[2]);
	double timestamp = Convert::ToDouble(arguments[5]);

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (service->GetState() == StateOK)
		BOOST_THROW_EXCEPTION(invalid_argument("The service '" + arguments[1] + "' is OK."));

	Logger::Write(LogInformation, "icinga", "Setting timed acknowledgement for service '" + service->GetName() + "'");
	service->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
	service->SetAcknowledgementExpiry(timestamp);
}

void ExternalCommandProcessor::RemoveSvcAcknowledgement(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Removing acknowledgement for service '" + service->GetName() + "'");
	service->SetAcknowledgement(AcknowledgementNone);
	service->SetAcknowledgementExpiry(0);
}

void ExternalCommandProcessor::AcknowledgeHostProblem(double, const vector<String>& arguments)
{
	if (arguments.size() < 6)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 6 arguments."));

	bool sticky = Convert::ToBool(arguments[1]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Setting acknowledgement for host '" + host->GetName() + "'");
	Service::Ptr service = host->GetHostCheckService();
	if (service) {
		if (service->GetState() == StateOK)
			BOOST_THROW_EXCEPTION(invalid_argument("The host '" + arguments[0] + "' is OK."));

		service->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
		service->SetAcknowledgementExpiry(0);
	}
}

void ExternalCommandProcessor::AcknowledgeHostProblemExpire(double, const vector<String>& arguments)
{
	if (arguments.size() < 7)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 7 arguments."));

	bool sticky = Convert::ToBool(arguments[1]);
	double timestamp = Convert::ToDouble(arguments[4]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Setting timed acknowledgement for host '" + host->GetName() + "'");
	Service::Ptr service = host->GetHostCheckService();
	if (service) {
		if (service->GetState() == StateOK)
			BOOST_THROW_EXCEPTION(invalid_argument("The host '" + arguments[0] + "' is OK."));

		service->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
		service->SetAcknowledgementExpiry(timestamp);
	}
}

void ExternalCommandProcessor::RemoveHostAcknowledgement(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Removing acknowledgement for host '" + host->GetName() + "'");
	Service::Ptr service = host->GetHostCheckService();
	if (service) {
		service->SetAcknowledgement(AcknowledgementNone);
		service->SetAcknowledgementExpiry(0);
	}
}

void ExternalCommandProcessor::EnableHostgroupSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, HostGroup::GetMembers(hg)) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + service->GetName() + "'");
			service->SetEnableActiveChecks(true);
		}
	}
}

void ExternalCommandProcessor::DisableHostgroupSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, HostGroup::GetMembers(hg)) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + service->GetName() + "'");
			service->SetEnableActiveChecks(false);
		}
	}
}

void ExternalCommandProcessor::EnableServicegroupSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, ServiceGroup::GetMembers(sg)) {
		Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(true);
	}
}

void ExternalCommandProcessor::DisableServicegroupSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, ServiceGroup::GetMembers(sg)) {
		Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(false);
	}
}

void ExternalCommandProcessor::EnablePassiveSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Enabling passive checks for service '" + arguments[1] + "'");
	service->SetEnablePassiveChecks(true);
}

void ExternalCommandProcessor::DisablePassiveSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Disabling passive checks for service '" + arguments[1] + "'");
	service->SetEnablePassiveChecks(false);
}

void ExternalCommandProcessor::EnableServicegroupPassiveSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, ServiceGroup::GetMembers(sg)) {
		Logger::Write(LogInformation, "icinga", "Enabling passive checks for service '" + service->GetName() + "'");
		service->SetEnablePassiveChecks(true);
	}
}

void ExternalCommandProcessor::DisableServicegroupPassiveSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, ServiceGroup::GetMembers(sg)) {
		Logger::Write(LogInformation, "icinga", "Disabling passive checks for service '" + service->GetName() + "'");
		service->SetEnablePassiveChecks(true);
	}
}

void ExternalCommandProcessor::EnableHostgroupPassiveSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, HostGroup::GetMembers(hg)) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Enabling passive checks for service '" + service->GetName() + "'");
			service->SetEnablePassiveChecks(true);
		}
	}
}

void ExternalCommandProcessor::DisableHostgroupPassiveSvcChecks(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, HostGroup::GetMembers(hg)) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Disabling passive checks for service '" + service->GetName() + "'");
			service->SetEnablePassiveChecks(false);
		}
	}
}

void ExternalCommandProcessor::ProcessFile(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	String file = arguments[0];
	bool del = Convert::ToBool(arguments[1]);

	ifstream ifp;
	ifp.exceptions(ifstream::badbit);

	ifp.open(file.CStr(), ifstream::in);

	while(ifp.good()) {
		std::string line;
		std::getline(ifp, line);

		try {
			Logger::Write(LogInformation, "compat", "Executing external command: " + line);

			Execute(line);
		} catch (const exception& ex) {
			stringstream msgbuf;
			msgbuf << "External command failed: " << diagnostic_information(ex);
			Logger::Write(LogWarning, "icinga", msgbuf.str());
		}
	}

	ifp.close();

	if (del)
		(void) unlink(file.CStr());
}

void ExternalCommandProcessor::ScheduleSvcDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 9)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 9 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[5]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Logger::Write(LogInformation, "icinga", "Creating downtime for service " + service->GetName());
	(void) service->AddDowntime(arguments[7], arguments[8],
	    Convert::ToDouble(arguments[2]), Convert::ToDouble(arguments[3]),
	    Convert::ToBool(arguments[4]), triggeredBy, Convert::ToDouble(arguments[6]));
}

void ExternalCommandProcessor::DelSvcDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	int id = Convert::ToLong(arguments[0]);
	Logger::Write(LogInformation, "icinga", "Removing downtime ID " + arguments[0]);
	String rid = Service::GetDowntimeIDFromLegacyID(id);
	Service::RemoveDowntime(rid);
}

void ExternalCommandProcessor::ScheduleHostDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 arguments."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Logger::Write(LogInformation, "icinga", "Creating downtime for host " + host->GetName());
	Service::Ptr service = host->GetHostCheckService();
	if (service) {
		(void) service->AddDowntime(arguments[6], arguments[7],
		    Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		    Convert::ToBool(arguments[3]), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::DelHostDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	int id = Convert::ToLong(arguments[0]);
	Logger::Write(LogInformation, "icinga", "Removing downtime ID " + arguments[0]);
	String rid = Service::GetDowntimeIDFromLegacyID(id);
	Service::RemoveDowntime(rid);
}

void ExternalCommandProcessor::ScheduleHostSvcDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 argument."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		Logger::Write(LogInformation, "icinga", "Creating downtime for service " + service->GetName());
		(void) service->AddDowntime(arguments[6], arguments[7],
		    Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		    Convert::ToBool(arguments[3]), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleHostgroupHostDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 arguments."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	BOOST_FOREACH(const Host::Ptr& host, HostGroup::GetMembers(hg)) {
		Logger::Write(LogInformation, "icinga", "Creating downtime for host " + host->GetName());
		Service::Ptr service = host->GetHostCheckService();
		if (service) {
			(void) service->AddDowntime(arguments[6], arguments[7],
			    Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			    Convert::ToBool(arguments[3]), triggeredBy, Convert::ToDouble(arguments[5]));
		}
	}
}

void ExternalCommandProcessor::ScheduleHostgroupSvcDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 arguments."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	/* Note: we can't just directly create downtimes for all the services by iterating
	 * over all hosts in the host group - otherwise we might end up creating multiple
	 * downtimes for some services. */

	set<Service::Ptr> services;

	BOOST_FOREACH(const Host::Ptr& host, HostGroup::GetMembers(hg)) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			services.insert(service);
		}
	}

	BOOST_FOREACH(const Service::Ptr& service, services) {
		Logger::Write(LogInformation, "icinga", "Creating downtime for service " + service->GetName());
		(void) service->AddDowntime(arguments[6], arguments[7],
		    Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		    Convert::ToBool(arguments[3]), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleServicegroupHostDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 arguments."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	/* Note: we can't just directly create downtimes for all the hosts by iterating
	 * over all services in the service group - otherwise we might end up creating multiple
	 * downtimes for some hosts. */

	set<Service::Ptr> services;

	BOOST_FOREACH(const Service::Ptr& service, ServiceGroup::GetMembers(sg)) {
		Host::Ptr host = service->GetHost();
		Service::Ptr hcService = host->GetHostCheckService();
		if (hcService)
			services.insert(hcService);
	}

	BOOST_FOREACH(const Service::Ptr& service, services) {
		Logger::Write(LogInformation, "icinga", "Creating downtime for service " + service->GetName());
		(void) service->AddDowntime(arguments[6], arguments[7],
		    Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		    Convert::ToBool(arguments[3]), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleServicegroupSvcDowntime(double, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 8 arguments."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	BOOST_FOREACH(const Service::Ptr& service, ServiceGroup::GetMembers(sg)) {
		Logger::Write(LogInformation, "icinga", "Creating downtime for service " + service->GetName());
		(void) service->AddDowntime(arguments[6], arguments[7],
		    Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		    Convert::ToBool(arguments[3]), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::AddHostComment(double, const vector<String>& arguments)
{
	if (arguments.size() < 4)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 4 arguments."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Creating comment for host " + host->GetName());
	Service::Ptr service = host->GetHostCheckService();
	if (service)
		(void) service->AddComment(CommentUser, arguments[2], arguments[3], 0);
}

void ExternalCommandProcessor::DelHostComment(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	int id = Convert::ToLong(arguments[0]);
	Logger::Write(LogInformation, "icinga", "Removing comment ID " + arguments[0]);
	String rid = Service::GetCommentIDFromLegacyID(id);
	Service::RemoveComment(rid);
}

void ExternalCommandProcessor::AddSvcComment(double, const vector<String>& arguments)
{
	if (arguments.size() < 5)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 5 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Creating comment for service " + service->GetName());
	(void) service->AddComment(CommentUser, arguments[3], arguments[4], 0);
}

void ExternalCommandProcessor::DelSvcComment(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	int id = Convert::ToLong(arguments[0]);
	Logger::Write(LogInformation, "icinga", "Removing comment ID " + arguments[0]);

	String rid = Service::GetCommentIDFromLegacyID(id);
	Service::RemoveComment(rid);
}

void ExternalCommandProcessor::DelAllHostComments(double, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 1 argument."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Removing all comments for host " + host->GetName());
	Service::Ptr service = host->GetHostCheckService();
	if (service)
		service->RemoveAllComments();
}

void ExternalCommandProcessor::DelAllSvcComments(double, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 2 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Removing all comments for service " + service->GetName());
	service->RemoveAllComments();
}

void ExternalCommandProcessor::SendCustomHostNotification(double time, const vector<String>& arguments)
{
	if (arguments.size() < 4)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 4 arguments."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Sending custom notification for host " + host->GetName());
	Service::Ptr service = host->GetHostCheckService();
	if (service) {
		service->RequestNotifications(NotificationCustom);
	}
}

void ExternalCommandProcessor::SendCustomSvcNotification(double time, const vector<String>& arguments)
{
	if (arguments.size() < 5)
		BOOST_THROW_EXCEPTION(invalid_argument("Expected 5 arguments."));

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	Logger::Write(LogInformation, "icinga", "Sending custom notification for service " + service->GetName());
	service->RequestNotifications(NotificationCustom);
}
