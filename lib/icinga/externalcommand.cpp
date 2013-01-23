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
		RegisterCommand("REMOVE_SVC_ACKNOWLEDGEMENT", &ExternalCommand::RemoveSvcAcknowledgement);


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

	Logger::Write(LogInformation, "icinga", "Enabling checks for service '" + arguments[1] + "'");
	service->SetEnableChecks(true);
}

void ExternalCommand::DisableSvcCheck(double time, const vector<String>& arguments)
{
	if (arguments.size() < 2)
		throw_exception(invalid_argument("Expected 2 arguments."));

	if (!Service::Exists(arguments[1]))
		throw_exception(invalid_argument("The service '" + arguments[1] + "' does not exist."));

	Service::Ptr service = Service::GetByName(arguments[1]);

	Logger::Write(LogInformation, "icinga", "Disabling checks for service '" + arguments[1] + "'");
	service->SetEnableChecks(false);
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

		Logger::Write(LogInformation, "icinga", "Enabling checks for service '" + service->GetName() + "'");
		service->SetEnableChecks(true);
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

		Logger::Write(LogInformation, "icinga", "Disabling checks for service '" + service->GetName() + "'");
		service->SetEnableChecks(false);
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

