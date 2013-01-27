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

bool I2_EXPORT ExternalCommand::m_Initialized;
map<String, ExternalCommand::Callback> I2_EXPORT ExternalCommand::m_Commands;

void ExternalCommand::Execute(const String& line)
{
	if (line.IsEmpty())
		return;

	if (line[0] != '[')
		throw_exception(invalid_argument("Missing timestamp in command: " + line));

	size_t pos = line.FindFirstOf("]");

	if (pos == String::NPos)
		throw_exception(invalid_argument("Missing timestamp in command: " + line));

	String timestamp = line.SubStr(1, pos - 1);
	String args = line.SubStr(pos + 2, String::NPos);

	double ts = timestamp.ToDouble();

	if (ts == 0)
		throw_exception(invalid_argument("Invalid timestamp in command: " + line));

	vector<String> argv = args.Split(is_any_of(";"));

	if (argv.size() == 0)
		throw_exception(invalid_argument("Missing arguments in command: " + line));

	vector<String> argvExtra(argv.begin() + 1, argv.end());
	Execute(ts, argv[0], argvExtra);
}

void ExternalCommand::Execute(double time, const String& command, const vector<String>& arguments)
{
	if (!m_Initialized) {
		RegisterCommand("HELLO_WORLD", &ExternalCommand::HelloWorld);
		RegisterCommand("PROCESS_SERVICE_CHECK_RESULT", &ExternalCommand::ProcessServiceCheckResult);
		RegisterCommand("SCHEDULE_SVC_CHECK", &ExternalCommand::ScheduleSvcCheck);
		RegisterCommand("SCHEDULE_FORCED_SVC_CHECK", &ExternalCommand::ScheduleForcedSvcCheck);
		RegisterCommand("ENABLE_SVC_CHECK", &ExternalCommand::EnableSvcCheck);
		RegisterCommand("DISABLE_SVC_CHECK", &ExternalCommand::DisableSvcCheck);
		RegisterCommand("SHUTDOWN_PROCESS", &ExternalCommand::ShutdownProcess);
		RegisterCommand("SCHEDULE_FORCED_HOST_SVC_CHECKS", &ExternalCommand::ScheduleForcedHostSvcChecks);
		RegisterCommand("SCHEDULE_HOST_SVC_CHECKS", &ExternalCommand::ScheduleHostSvcChecks);
		RegisterCommand("ENABLE_HOST_SVC_CHECKS", &ExternalCommand::EnableHostSvcChecks);
		RegisterCommand("DISABLE_HOST_SVC_CHECKS", &ExternalCommand::DisableHostSvcChecks);
		RegisterCommand("ACKNOWLEDGE_SVC_PROBLEM", &ExternalCommand::AcknowledgeSvcProblem);
		RegisterCommand("ACKNOWLEDGE_SVC_PROBLEM_EXPIRE", &ExternalCommand::AcknowledgeSvcProblemExpire);
		RegisterCommand("REMOVE_SVC_ACKNOWLEDGEMENT", &ExternalCommand::RemoveHostAcknowledgement);
		RegisterCommand("ACKNOWLEDGE_HOST_PROBLEM", &ExternalCommand::AcknowledgeHostProblem);
		RegisterCommand("ACKNOWLEDGE_HOST_PROBLEM_EXPIRE", &ExternalCommand::AcknowledgeHostProblemExpire);
		RegisterCommand("REMOVE_HOST_ACKNOWLEDGEMENT", &ExternalCommand::RemoveHostAcknowledgement);
		RegisterCommand("ENABLE_HOSTGROUP_SVC_CHECKS", &ExternalCommand::EnableHostgroupSvcChecks);
		RegisterCommand("DISABLE_HOSTGROUP_SVC_CHECKS", &ExternalCommand::DisableHostgroupSvcChecks);
		RegisterCommand("ENABLE_SERVICEGROUP_SVC_CHECKS", &ExternalCommand::EnableServicegroupSvcChecks);
		RegisterCommand("DISABLE_SERVICEGROUP_SVC_CHECKS", &ExternalCommand::DisableServicegroupSvcChecks);
		RegisterCommand("ENABLE_PASSIVE_SVC_CHECKS", &ExternalCommand::EnablePassiveSvcChecks);
		RegisterCommand("DISABLE_PASSIVE_SVC_CHECKS", &ExternalCommand::DisablePassiveSvcChecks);
		RegisterCommand("ENABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS", &ExternalCommand::EnableServicegroupPassiveSvcChecks);
		RegisterCommand("DISABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS", &ExternalCommand::DisableServicegroupPassiveSvcChecks);
		RegisterCommand("ENABLE_HOSTGROUP_PASSIVE_SVC_CHECKS", &ExternalCommand::EnableHostgroupPassiveSvcChecks);
		RegisterCommand("DISABLE_HOSTGROUP_PASSIVE_SVC_CHECKS", &ExternalCommand::DisableHostgroupPassiveSvcChecks);
		RegisterCommand("PROCESS_FILE", &ExternalCommand::ProcessFile);

		m_Initialized = true;
	}

	map<String, ExternalCommand::Callback>::iterator it;
	it = m_Commands.find(command);

	if (it == m_Commands.end())
		throw_exception(invalid_argument("The external command '" + command + "' does not exist."));

	it->second(time, arguments);
}

void ExternalCommand::RegisterCommand(const String& command, const ExternalCommand::Callback& callback)
{
	m_Commands[command] = callback;
}

void ExternalCommand::HelloWorld(double time, const vector<String>& arguments)
{
	Logger::Write(LogInformation, "icinga", "HelloWorld external command called.");
}

void ExternalCommand::ProcessServiceCheckResult(double time, const vector<String>& arguments)
{
	if (arguments.size() < 4)
		throw_exception(invalid_argument("Expected 4 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	if (!service->GetEnablePassiveChecks())
		throw_exception(invalid_argument("Got passive check result for service '" + arguments[1] + "' which has passive checks disabled."));

	int exitStatus = arguments[2].ToDouble();
	Dictionary::Ptr result = PluginCheckTask::ParseCheckOutput(arguments[3]);
	result->Set("state", PluginCheckTask::ExitStatusToState(exitStatus));

	double now = Utility::GetTime();
	result->Set("schedule_start", time);
	result->Set("schedule_end", time);
	result->Set("execution_start", time);
	result->Set("execution_end", time);

	Logger::Write(LogInformation, "icinga", "Processing passive check result for service '" + arguments[1] + "'");
	service->ProcessCheckResult(result);
}

void ExternalCommand::ScheduleSvcCheck(double time, const vector<String>& arguments)
{
	if (arguments.size() < 3)
		throw_exception(invalid_argument("Expected 3 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	double planned_check = arguments[2].ToDouble();

	if (planned_check > service->GetNextCheck()) {
		Logger::Write(LogInformation, "icinga", "Ignoring reschedule request for service '" +
		    arguments[1] + "' (next check is already sooner than requested check time)");
		return;
	}

	Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + arguments[1] + "'");
	service->SetNextCheck(planned_check);
}

void ExternalCommand::ScheduleForcedSvcCheck(double time, const vector<String>& arguments)
{
	if (arguments.size() < 3)
		throw_exception(invalid_argument("Expected 3 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + arguments[1] + "'");
	service->SetForceNextCheck(true);
	service->SetNextCheck(arguments[2].ToDouble());
}

void ExternalCommand::EnableSvcCheck(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + arguments[1] + "'");
	service->SetEnableActiveChecks(true);
}

void ExternalCommand::DisableSvcCheck(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + arguments[1] + "'");
	service->SetEnableActiveChecks(false);
}

void ExternalCommand::ShutdownProcess(double time, const vector<String>& arguments)
{
	Logger::Write(LogInformation, "icinga", "Shutting down Icinga via external command.");
	Application::RequestShutdown();
}

void ExternalCommand::ScheduleForcedHostSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	double planned_check = arguments[1].ToDouble();

	Host::Ptr host = Host::GetByName(arguments[0]);

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		if (service->GetHost() != host)
			continue;

		Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + service->GetName() + "'");
		service->SetNextCheck(planned_check);
		service->SetForceNextCheck(true);
	}
}

void ExternalCommand::ScheduleHostSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	double planned_check = arguments[1].ToDouble();

	Host::Ptr host = Host::GetByName(arguments[0]);

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		if (service->GetHost() != host)
			continue;

		if (planned_check > service->GetNextCheck()) {
			Logger::Write(LogInformation, "icinga", "Ignoring reschedule request for service '" +
			    service->GetName() + "' (next check is already sooner than requested check time)");
			continue;
		}

		Logger::Write(LogInformation, "icinga", "Rescheduling next check for service '" + service->GetName() + "'");
		service->SetNextCheck(planned_check);
	}
}

void ExternalCommand::EnableHostSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		if (service->GetHost() != host)
			continue;

		Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(true);
	}
}

void ExternalCommand::DisableHostSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 arguments."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		if (service->GetHost() != host)
			continue;

		Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(false);
	}
}

void ExternalCommand::AcknowledgeSvcProblem(double time, const vector<String>& arguments)
{
	if (arguments.size() < 7)
		throw_exception(invalid_argument("Expected 7 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	int sticky = arguments[2].ToDouble();

	Service::Ptr service = Service::GetByName(arguments[1]);

	if (service->GetState() == StateOK)
		throw_exception(invalid_argument("The service '" + arguments[1] + "' is OK."));

	Logger::Write(LogInformation, "icinga", "Setting acknowledgement for service '" + service->GetName() + "'");
	service->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
	service->SetAcknowledgementExpiry(0);
}

void ExternalCommand::AcknowledgeSvcProblemExpire(double time, const vector<String>& arguments)
{
	if (arguments.size() < 8)
		throw_exception(invalid_argument("Expected 8 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	int sticky = arguments[2].ToDouble();
	double timestamp = arguments[5].ToDouble();

	Service::Ptr service = Service::GetByName(arguments[1]);

	if (service->GetState() == StateOK)
		throw_exception(invalid_argument("The service '" + arguments[1] + "' is OK."));

	Logger::Write(LogInformation, "icinga", "Setting timed acknowledgement for service '" + service->GetName() + "'");
	service->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
	service->SetAcknowledgementExpiry(timestamp);
}

void ExternalCommand::RemoveSvcAcknowledgement(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Removing acknowledgement for service '" + service->GetName() + "'");
	service->SetAcknowledgement(AcknowledgementNone);
	service->SetAcknowledgementExpiry(0);
}

void ExternalCommand::AcknowledgeHostProblem(double time, const vector<String>& arguments)
{
	if (arguments.size() < 6)
		throw_exception(invalid_argument("Expected 6 arguments."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	int sticky = arguments[0].ToDouble();

	Host::Ptr host = Host::GetByName(arguments[0]);

	if (host->IsUp())
		throw_exception(invalid_argument("The host '" + arguments[0] + "' is OK."));

	Logger::Write(LogInformation, "icinga", "Setting acknowledgement for host '" + host->GetName() + "'");
	host->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
	host->SetAcknowledgementExpiry(0);
}

void ExternalCommand::AcknowledgeHostProblemExpire(double time, const vector<String>& arguments)
{
	if (arguments.size() < 7)
		throw_exception(invalid_argument("Expected 7 arguments."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	int sticky = arguments[1].ToDouble();
	double timestamp = arguments[4].ToDouble();

	Host::Ptr host = Host::GetByName(arguments[0]);

	if (host->IsUp())
		throw_exception(invalid_argument("The host '" + arguments[0] + "' is OK."));

	Logger::Write(LogInformation, "icinga", "Setting timed acknowledgement for host '" + host->GetName() + "'");
	host->SetAcknowledgement(sticky ? AcknowledgementSticky : AcknowledgementNormal);
	host->SetAcknowledgementExpiry(timestamp);
}

void ExternalCommand::RemoveHostAcknowledgement(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!Host::Exists(arguments[0]))
		throw_exception(invalid_argument("The host '" + arguments[0] + "' does not exist."));

	Host::Ptr host = Host::GetByName(arguments[0]);

	Logger::Write(LogInformation, "icinga", "Removing acknowledgement for host '" + host->GetName() + "'");
	host->SetAcknowledgement(AcknowledgementNone);
	host->SetAcknowledgementExpiry(0);
}

void ExternalCommand::EnableHostgroupSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!HostGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The host group '" + arguments[0] + "' does not exist."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, hg->GetMembers()) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + service->GetName() + "'");
			service->SetEnableActiveChecks(true);
		}
	}
}

void ExternalCommand::DisableHostgroupSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!HostGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The host group '" + arguments[0] + "' does not exist."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, hg->GetMembers()) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + service->GetName() + "'");
			service->SetEnableActiveChecks(false);
		}
	}
}

void ExternalCommand::EnableServicegroupSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!ServiceGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The service group '" + arguments[0] + "' does not exist."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
		Logger::Write(LogInformation, "icinga", "Enabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(true);
	}
}

void ExternalCommand::DisableServicegroupSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!ServiceGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The service group '" + arguments[0] + "' does not exist."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
		Logger::Write(LogInformation, "icinga", "Disabling active checks for service '" + service->GetName() + "'");
		service->SetEnableActiveChecks(false);
	}
}

void ExternalCommand::EnablePassiveSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Enabling passive checks for service '" + arguments[1] + "'");
	service->SetEnablePassiveChecks(true);
}

void ExternalCommand::DisablePassiveSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Disabling passive checks for service '" + arguments[1] + "'");
	service->SetEnablePassiveChecks(false);
}

void ExternalCommand::EnableServicegroupPassiveSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!ServiceGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The service group '" + arguments[0] + "' does not exist."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
		Logger::Write(LogInformation, "icinga", "Enabling passive checks for service '" + service->GetName() + "'");
		service->SetEnablePassiveChecks(true);
	}
}

void ExternalCommand::DisableServicegroupPassiveSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!ServiceGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The service group '" + arguments[0] + "' does not exist."));

	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
		Logger::Write(LogInformation, "icinga", "Disabling passive checks for service '" + service->GetName() + "'");
		service->SetEnablePassiveChecks(true);
	}
}

void ExternalCommand::EnableHostgroupPassiveSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!HostGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The host group '" + arguments[0] + "' does not exist."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, hg->GetMembers()) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Enabling passive checks for service '" + service->GetName() + "'");
			service->SetEnablePassiveChecks(true);
		}
	}
}

void ExternalCommand::DisableHostgroupPassiveSvcChecks(double time, const vector<String>& arguments)
{
	if (arguments.size() < 1)
		throw_exception(invalid_argument("Expected 1 argument."));

	if (!HostGroup::Exists(arguments[0]))
		throw_exception(invalid_argument("The host group '" + arguments[0] + "' does not exist."));

	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	BOOST_FOREACH(const Host::Ptr& host, hg->GetMembers()) {
		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			Logger::Write(LogInformation, "icinga", "Disabling passive checks for service '" + service->GetName() + "'");
			service->SetEnablePassiveChecks(false);
		}
	}
}

void ExternalCommand::ProcessFile(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	String file = arguments[0];
	int del = arguments[1].ToDouble();

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
			msgbuf << "External command failed: " << ex.what();
			Logger::Write(LogWarning, "icinga", msgbuf.str());
		}
	}

	ifp.close();

	if (del)
		(void) unlink(file.CStr());
}
