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

#include "icinga/externalcommandprocessor.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/user.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/compatutility.hpp"
#include "remote/apifunction.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include <fstream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread/once.hpp>

using namespace icinga;

boost::signals2::signal<void(double, const String&, const std::vector<String>&)> ExternalCommandProcessor::OnNewExternalCommand;

void ExternalCommandProcessor::Execute(const String& line)
{
	if (line.IsEmpty())
		return;

	if (line[0] != '[')
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing timestamp in command: " + line));

	size_t pos = line.FindFirstOf("]");

	if (pos == String::NPos)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing timestamp in command: " + line));

	String timestamp = line.SubStr(1, pos - 1);
	String args = line.SubStr(pos + 2, String::NPos);

	double ts = Convert::ToDouble(timestamp);

	if (ts == 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid timestamp in command: " + line));

	std::vector<String> argv;
	boost::algorithm::split(argv, args, boost::is_any_of(";"));

	if (argv.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing arguments in command: " + line));

	std::vector<String> argvExtra(argv.begin() + 1, argv.end());
	Execute(ts, argv[0], argvExtra);
}

void ExternalCommandProcessor::Execute(double time, const String& command, const std::vector<String>& arguments)
{
	ExternalCommandInfo eci;

	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, []() {
		RegisterCommands();
	});

	{
		boost::mutex::scoped_lock lock(GetMutex());

		auto it = GetCommands().find(command);

		if (it == GetCommands().end())
			BOOST_THROW_EXCEPTION(std::invalid_argument("The external command '" + command + "' does not exist."));

		eci = it->second;
	}

	if (arguments.size() < eci.MinArgs)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Expected " + Convert::ToString(eci.MinArgs) + " arguments"));

	size_t argnum = std::min(arguments.size(), eci.MaxArgs);

	std::vector<String> realArguments;
	realArguments.resize(argnum);

	if (argnum > 0) {
		std::copy(arguments.begin(), arguments.begin() + argnum - 1, realArguments.begin());

		String last_argument;
		for (std::vector<String>::size_type i = argnum - 1; i < arguments.size(); i++) {
			if (!last_argument.IsEmpty())
				last_argument += ";";

			last_argument += arguments[i];
		}

		realArguments[argnum - 1] = last_argument;
	}

	OnNewExternalCommand(time, command, realArguments);

	eci.Callback(time, realArguments);
}

void ExternalCommandProcessor::RegisterCommand(const String& command, const ExternalCommandCallback& callback, size_t minArgs, size_t maxArgs)
{
	boost::mutex::scoped_lock lock(GetMutex());
	ExternalCommandInfo eci;
	eci.Callback = callback;
	eci.MinArgs = minArgs;
	eci.MaxArgs = (maxArgs == UINT_MAX) ? minArgs : maxArgs;
	GetCommands()[command] = eci;
}

void ExternalCommandProcessor::RegisterCommands()
{
	RegisterCommand("PROCESS_HOST_CHECK_RESULT", &ExternalCommandProcessor::ProcessHostCheckResult, 3);
	RegisterCommand("PROCESS_SERVICE_CHECK_RESULT", &ExternalCommandProcessor::ProcessServiceCheckResult, 4);
	RegisterCommand("SCHEDULE_HOST_CHECK", &ExternalCommandProcessor::ScheduleHostCheck, 2);
	RegisterCommand("SCHEDULE_FORCED_HOST_CHECK", &ExternalCommandProcessor::ScheduleForcedHostCheck, 2);
	RegisterCommand("SCHEDULE_SVC_CHECK", &ExternalCommandProcessor::ScheduleSvcCheck, 3);
	RegisterCommand("SCHEDULE_FORCED_SVC_CHECK", &ExternalCommandProcessor::ScheduleForcedSvcCheck, 3);
	RegisterCommand("ENABLE_HOST_CHECK", &ExternalCommandProcessor::EnableHostCheck, 1);
	RegisterCommand("DISABLE_HOST_CHECK", &ExternalCommandProcessor::DisableHostCheck, 1);
	RegisterCommand("ENABLE_SVC_CHECK", &ExternalCommandProcessor::EnableSvcCheck, 2);
	RegisterCommand("DISABLE_SVC_CHECK", &ExternalCommandProcessor::DisableSvcCheck, 2);
	RegisterCommand("SHUTDOWN_PROCESS", &ExternalCommandProcessor::ShutdownProcess);
	RegisterCommand("RESTART_PROCESS", &ExternalCommandProcessor::RestartProcess);
	RegisterCommand("SCHEDULE_FORCED_HOST_SVC_CHECKS", &ExternalCommandProcessor::ScheduleForcedHostSvcChecks, 2);
	RegisterCommand("SCHEDULE_HOST_SVC_CHECKS", &ExternalCommandProcessor::ScheduleHostSvcChecks, 2);
	RegisterCommand("ENABLE_HOST_SVC_CHECKS", &ExternalCommandProcessor::EnableHostSvcChecks, 1);
	RegisterCommand("DISABLE_HOST_SVC_CHECKS", &ExternalCommandProcessor::DisableHostSvcChecks, 1);
	RegisterCommand("ACKNOWLEDGE_SVC_PROBLEM", &ExternalCommandProcessor::AcknowledgeSvcProblem, 7);
	RegisterCommand("ACKNOWLEDGE_SVC_PROBLEM_EXPIRE", &ExternalCommandProcessor::AcknowledgeSvcProblemExpire, 8);
	RegisterCommand("REMOVE_SVC_ACKNOWLEDGEMENT", &ExternalCommandProcessor::RemoveSvcAcknowledgement, 2);
	RegisterCommand("ACKNOWLEDGE_HOST_PROBLEM", &ExternalCommandProcessor::AcknowledgeHostProblem, 6);
	RegisterCommand("ACKNOWLEDGE_HOST_PROBLEM_EXPIRE", &ExternalCommandProcessor::AcknowledgeHostProblemExpire, 7);
	RegisterCommand("REMOVE_HOST_ACKNOWLEDGEMENT", &ExternalCommandProcessor::RemoveHostAcknowledgement, 1);
	RegisterCommand("DISABLE_HOST_FLAP_DETECTION", &ExternalCommandProcessor::DisableHostFlapping, 1);
	RegisterCommand("ENABLE_HOST_FLAP_DETECTION", &ExternalCommandProcessor::EnableHostFlapping, 1);
	RegisterCommand("DISABLE_SVC_FLAP_DETECTION", &ExternalCommandProcessor::DisableSvcFlapping, 2);
	RegisterCommand("ENABLE_SVC_FLAP_DETECTION", &ExternalCommandProcessor::EnableSvcFlapping, 2);
	RegisterCommand("ENABLE_HOSTGROUP_SVC_CHECKS", &ExternalCommandProcessor::EnableHostgroupSvcChecks, 1);
	RegisterCommand("DISABLE_HOSTGROUP_SVC_CHECKS", &ExternalCommandProcessor::DisableHostgroupSvcChecks, 1);
	RegisterCommand("ENABLE_SERVICEGROUP_SVC_CHECKS", &ExternalCommandProcessor::EnableServicegroupSvcChecks, 1);
	RegisterCommand("DISABLE_SERVICEGROUP_SVC_CHECKS", &ExternalCommandProcessor::DisableServicegroupSvcChecks, 1);
	RegisterCommand("ENABLE_PASSIVE_HOST_CHECKS", &ExternalCommandProcessor::EnablePassiveHostChecks, 1);
	RegisterCommand("DISABLE_PASSIVE_HOST_CHECKS", &ExternalCommandProcessor::DisablePassiveHostChecks, 1);
	RegisterCommand("ENABLE_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::EnablePassiveSvcChecks, 2);
	RegisterCommand("DISABLE_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::DisablePassiveSvcChecks, 2);
	RegisterCommand("ENABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::EnableServicegroupPassiveSvcChecks, 1);
	RegisterCommand("DISABLE_SERVICEGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::DisableServicegroupPassiveSvcChecks, 1);
	RegisterCommand("ENABLE_HOSTGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::EnableHostgroupPassiveSvcChecks, 1);
	RegisterCommand("DISABLE_HOSTGROUP_PASSIVE_SVC_CHECKS", &ExternalCommandProcessor::DisableHostgroupPassiveSvcChecks, 1);
	RegisterCommand("PROCESS_FILE", &ExternalCommandProcessor::ProcessFile, 2);
	RegisterCommand("SCHEDULE_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleSvcDowntime, 9);
	RegisterCommand("DEL_SVC_DOWNTIME", &ExternalCommandProcessor::DelSvcDowntime, 1);
	RegisterCommand("SCHEDULE_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleHostDowntime, 8);
	RegisterCommand("SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleAndPropagateHostDowntime, 8);
	RegisterCommand("SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleAndPropagateTriggeredHostDowntime, 8);
	RegisterCommand("DEL_HOST_DOWNTIME", &ExternalCommandProcessor::DelHostDowntime, 1);
	RegisterCommand("DEL_DOWNTIME_BY_HOST_NAME", &ExternalCommandProcessor::DelDowntimeByHostName, 1, 4);
	RegisterCommand("SCHEDULE_HOST_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleHostSvcDowntime, 8);
	RegisterCommand("SCHEDULE_HOSTGROUP_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleHostgroupHostDowntime, 8);
	RegisterCommand("SCHEDULE_HOSTGROUP_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleHostgroupSvcDowntime, 8);
	RegisterCommand("SCHEDULE_SERVICEGROUP_HOST_DOWNTIME", &ExternalCommandProcessor::ScheduleServicegroupHostDowntime, 8);
	RegisterCommand("SCHEDULE_SERVICEGROUP_SVC_DOWNTIME", &ExternalCommandProcessor::ScheduleServicegroupSvcDowntime, 8);
	RegisterCommand("ADD_HOST_COMMENT", &ExternalCommandProcessor::AddHostComment, 4);
	RegisterCommand("DEL_HOST_COMMENT", &ExternalCommandProcessor::DelHostComment, 1);
	RegisterCommand("ADD_SVC_COMMENT", &ExternalCommandProcessor::AddSvcComment, 5);
	RegisterCommand("DEL_SVC_COMMENT", &ExternalCommandProcessor::DelSvcComment, 1);
	RegisterCommand("DEL_ALL_HOST_COMMENTS", &ExternalCommandProcessor::DelAllHostComments, 1);
	RegisterCommand("DEL_ALL_SVC_COMMENTS", &ExternalCommandProcessor::DelAllSvcComments, 2);
	RegisterCommand("SEND_CUSTOM_HOST_NOTIFICATION", &ExternalCommandProcessor::SendCustomHostNotification, 4);
	RegisterCommand("SEND_CUSTOM_SVC_NOTIFICATION", &ExternalCommandProcessor::SendCustomSvcNotification, 5);
	RegisterCommand("DELAY_HOST_NOTIFICATION", &ExternalCommandProcessor::DelayHostNotification, 2);
	RegisterCommand("DELAY_SVC_NOTIFICATION", &ExternalCommandProcessor::DelaySvcNotification, 3);
	RegisterCommand("ENABLE_HOST_NOTIFICATIONS", &ExternalCommandProcessor::EnableHostNotifications, 1);
	RegisterCommand("DISABLE_HOST_NOTIFICATIONS", &ExternalCommandProcessor::DisableHostNotifications, 1);
	RegisterCommand("ENABLE_SVC_NOTIFICATIONS", &ExternalCommandProcessor::EnableSvcNotifications, 2);
	RegisterCommand("DISABLE_SVC_NOTIFICATIONS", &ExternalCommandProcessor::DisableSvcNotifications, 2);
	RegisterCommand("ENABLE_HOST_SVC_NOTIFICATIONS", &ExternalCommandProcessor::EnableHostSvcNotifications, 1);
	RegisterCommand("DISABLE_HOST_SVC_NOTIFICATIONS", &ExternalCommandProcessor::DisableHostSvcNotifications, 1);
	RegisterCommand("DISABLE_HOSTGROUP_HOST_CHECKS", &ExternalCommandProcessor::DisableHostgroupHostChecks, 1);
	RegisterCommand("DISABLE_HOSTGROUP_PASSIVE_HOST_CHECKS", &ExternalCommandProcessor::DisableHostgroupPassiveHostChecks, 1);
	RegisterCommand("DISABLE_SERVICEGROUP_HOST_CHECKS", &ExternalCommandProcessor::DisableServicegroupHostChecks, 1);
	RegisterCommand("DISABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS", &ExternalCommandProcessor::DisableServicegroupPassiveHostChecks, 1);
	RegisterCommand("ENABLE_HOSTGROUP_HOST_CHECKS", &ExternalCommandProcessor::EnableHostgroupHostChecks, 1);
	RegisterCommand("ENABLE_HOSTGROUP_PASSIVE_HOST_CHECKS", &ExternalCommandProcessor::EnableHostgroupPassiveHostChecks, 1);
	RegisterCommand("ENABLE_SERVICEGROUP_HOST_CHECKS", &ExternalCommandProcessor::EnableServicegroupHostChecks, 1);
	RegisterCommand("ENABLE_SERVICEGROUP_PASSIVE_HOST_CHECKS", &ExternalCommandProcessor::EnableServicegroupPassiveHostChecks, 1);
	RegisterCommand("ENABLE_NOTIFICATIONS", &ExternalCommandProcessor::EnableNotifications);
	RegisterCommand("DISABLE_NOTIFICATIONS", &ExternalCommandProcessor::DisableNotifications);
	RegisterCommand("ENABLE_FLAP_DETECTION", &ExternalCommandProcessor::EnableFlapDetection);
	RegisterCommand("DISABLE_FLAP_DETECTION", &ExternalCommandProcessor::DisableFlapDetection);
	RegisterCommand("ENABLE_EVENT_HANDLERS", &ExternalCommandProcessor::EnableEventHandlers);
	RegisterCommand("DISABLE_EVENT_HANDLERS", &ExternalCommandProcessor::DisableEventHandlers);
	RegisterCommand("ENABLE_PERFORMANCE_DATA", &ExternalCommandProcessor::EnablePerformanceData);
	RegisterCommand("DISABLE_PERFORMANCE_DATA", &ExternalCommandProcessor::DisablePerformanceData);
	RegisterCommand("START_EXECUTING_SVC_CHECKS", &ExternalCommandProcessor::StartExecutingSvcChecks);
	RegisterCommand("STOP_EXECUTING_SVC_CHECKS", &ExternalCommandProcessor::StopExecutingSvcChecks);
	RegisterCommand("START_EXECUTING_HOST_CHECKS", &ExternalCommandProcessor::StartExecutingHostChecks);
	RegisterCommand("STOP_EXECUTING_HOST_CHECKS", &ExternalCommandProcessor::StopExecutingHostChecks);
	RegisterCommand("CHANGE_NORMAL_SVC_CHECK_INTERVAL", &ExternalCommandProcessor::ChangeNormalSvcCheckInterval, 3);
	RegisterCommand("CHANGE_NORMAL_HOST_CHECK_INTERVAL", &ExternalCommandProcessor::ChangeNormalHostCheckInterval, 2);
	RegisterCommand("CHANGE_RETRY_SVC_CHECK_INTERVAL", &ExternalCommandProcessor::ChangeRetrySvcCheckInterval, 3);
	RegisterCommand("CHANGE_RETRY_HOST_CHECK_INTERVAL", &ExternalCommandProcessor::ChangeRetryHostCheckInterval, 2);
	RegisterCommand("ENABLE_HOST_EVENT_HANDLER", &ExternalCommandProcessor::EnableHostEventHandler, 1);
	RegisterCommand("DISABLE_HOST_EVENT_HANDLER", &ExternalCommandProcessor::DisableHostEventHandler, 1);
	RegisterCommand("ENABLE_SVC_EVENT_HANDLER", &ExternalCommandProcessor::EnableSvcEventHandler, 2);
	RegisterCommand("DISABLE_SVC_EVENT_HANDLER", &ExternalCommandProcessor::DisableSvcEventHandler, 2);
	RegisterCommand("CHANGE_HOST_EVENT_HANDLER", &ExternalCommandProcessor::ChangeHostEventHandler, 2);
	RegisterCommand("CHANGE_SVC_EVENT_HANDLER", &ExternalCommandProcessor::ChangeSvcEventHandler, 3);
	RegisterCommand("CHANGE_HOST_CHECK_COMMAND", &ExternalCommandProcessor::ChangeHostCheckCommand, 2);
	RegisterCommand("CHANGE_SVC_CHECK_COMMAND", &ExternalCommandProcessor::ChangeSvcCheckCommand, 3);
	RegisterCommand("CHANGE_MAX_HOST_CHECK_ATTEMPTS", &ExternalCommandProcessor::ChangeMaxHostCheckAttempts, 2);
	RegisterCommand("CHANGE_MAX_SVC_CHECK_ATTEMPTS", &ExternalCommandProcessor::ChangeMaxSvcCheckAttempts, 3);
	RegisterCommand("CHANGE_HOST_CHECK_TIMEPERIOD", &ExternalCommandProcessor::ChangeHostCheckTimeperiod, 2);
	RegisterCommand("CHANGE_SVC_CHECK_TIMEPERIOD", &ExternalCommandProcessor::ChangeSvcCheckTimeperiod, 3);
	RegisterCommand("CHANGE_CUSTOM_HOST_VAR", &ExternalCommandProcessor::ChangeCustomHostVar, 3);
	RegisterCommand("CHANGE_CUSTOM_SVC_VAR", &ExternalCommandProcessor::ChangeCustomSvcVar, 4);
	RegisterCommand("CHANGE_CUSTOM_USER_VAR", &ExternalCommandProcessor::ChangeCustomUserVar, 3);
	RegisterCommand("CHANGE_CUSTOM_CHECKCOMMAND_VAR", &ExternalCommandProcessor::ChangeCustomCheckcommandVar, 3);
	RegisterCommand("CHANGE_CUSTOM_EVENTCOMMAND_VAR", &ExternalCommandProcessor::ChangeCustomEventcommandVar, 3);
	RegisterCommand("CHANGE_CUSTOM_NOTIFICATIONCOMMAND_VAR", &ExternalCommandProcessor::ChangeCustomNotificationcommandVar, 3);

	RegisterCommand("ENABLE_HOSTGROUP_HOST_NOTIFICATIONS", &ExternalCommandProcessor::EnableHostgroupHostNotifications, 1);
	RegisterCommand("ENABLE_HOSTGROUP_SVC_NOTIFICATIONS", &ExternalCommandProcessor::EnableHostgroupSvcNotifications, 1);
	RegisterCommand("DISABLE_HOSTGROUP_HOST_NOTIFICATIONS", &ExternalCommandProcessor::DisableHostgroupHostNotifications, 1);
	RegisterCommand("DISABLE_HOSTGROUP_SVC_NOTIFICATIONS", &ExternalCommandProcessor::DisableHostgroupSvcNotifications, 1);
	RegisterCommand("ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS", &ExternalCommandProcessor::EnableServicegroupHostNotifications, 1);
	RegisterCommand("DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS", &ExternalCommandProcessor::DisableServicegroupHostNotifications, 1);
	RegisterCommand("ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS", &ExternalCommandProcessor::EnableServicegroupSvcNotifications, 1);
	RegisterCommand("DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS", &ExternalCommandProcessor::DisableServicegroupSvcNotifications, 1);
}

void ExternalCommandProcessor::ExecuteFromFile(const String& line, std::deque< std::vector<String> >& file_queue)
{
	if (line.IsEmpty())
		return;

	if (line[0] != '[')
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing timestamp in command: " + line));

	size_t pos = line.FindFirstOf("]");

	if (pos == String::NPos)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing timestamp in command: " + line));

	String timestamp = line.SubStr(1, pos - 1);
	String args = line.SubStr(pos + 2, String::NPos);

	double ts = Convert::ToDouble(timestamp);

	if (ts == 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid timestamp in command: " + line));

	std::vector<String> argv;
	boost::algorithm::split(argv, args, boost::is_any_of(";"));

	if (argv.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing arguments in command: " + line));

	std::vector<String> argvExtra(argv.begin() + 1, argv.end());

	if (argv[0] == "PROCESS_FILE") {
		Log(LogDebug, "ExternalCommandProcessor")
			<< "Enqueing external command file " << argvExtra[0];
		file_queue.push_back(argvExtra);
	} else {
		Execute(ts, argv[0], argvExtra);
	}
}

void ExternalCommandProcessor::ProcessHostCheckResult(double time, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot process passive host check result for non-existent host '" + arguments[0] + "'"));

	if (!host->GetEnablePassiveChecks())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Got passive check result for host '" + arguments[0] + "' which has passive checks disabled."));

	int exitStatus = Convert::ToDouble(arguments[1]);
	CheckResult::Ptr result = new CheckResult();
	std::pair<String, String> co = PluginUtility::ParseCheckOutput(arguments[2]);
	result->SetOutput(co.first);
	result->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));

	ServiceState state;

	if (exitStatus == 0)
		state = ServiceOK;
	else if (exitStatus == 1)
		state = ServiceCritical;
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid status code: " + arguments[1]));

	result->SetState(state);

	result->SetScheduleStart(time);
	result->SetScheduleEnd(time);
	result->SetExecutionStart(time);
	result->SetExecutionEnd(time);

	/* Mark this check result as passive. */
	result->SetActive(false);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Processing passive check result for host '" << arguments[0] << "'";

	host->ProcessCheckResult(result);
}

void ExternalCommandProcessor::ProcessServiceCheckResult(double time, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot process passive service check result for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	if (!service->GetEnablePassiveChecks())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Got passive check result for service '" + arguments[1] + "' which has passive checks disabled."));

	int exitStatus = Convert::ToDouble(arguments[2]);
	CheckResult::Ptr result = new CheckResult();
	String output = CompatUtility::UnEscapeString(arguments[3]);
	std::pair<String, String> co = PluginUtility::ParseCheckOutput(output);
	result->SetOutput(co.first);
	result->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
	result->SetState(PluginUtility::ExitStatusToState(exitStatus));

	result->SetScheduleStart(time);
	result->SetScheduleEnd(time);
	result->SetExecutionStart(time);
	result->SetExecutionEnd(time);

	/* Mark this check result as passive. */
	result->SetActive(false);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Processing passive check result for service '" << arguments[1] << "'";

	service->ProcessCheckResult(result);
}

void ExternalCommandProcessor::ScheduleHostCheck(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot reschedule host check for non-existent host '" + arguments[0] + "'"));

	double planned_check = Convert::ToDouble(arguments[1]);

	if (planned_check > host->GetNextCheck()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Ignoring reschedule request for host '"
			<< arguments[0] << "' (next check is already sooner than requested check time)";
		return;
	}

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Rescheduling next check for host '" << arguments[0] << "'";

	if (planned_check < Utility::GetTime())
		planned_check = Utility::GetTime();

	host->SetNextCheck(planned_check);

	/* trigger update event for DB IDO */
	Checkable::OnNextCheckUpdated(host);
}

void ExternalCommandProcessor::ScheduleForcedHostCheck(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot reschedule forced host check for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Rescheduling next check for host '" << arguments[0] << "'";

	host->SetForceNextCheck(true);
	host->SetNextCheck(Convert::ToDouble(arguments[1]));

	/* trigger update event for DB IDO */
	Checkable::OnNextCheckUpdated(host);
}

void ExternalCommandProcessor::ScheduleSvcCheck(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot reschedule service check for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	double planned_check = Convert::ToDouble(arguments[2]);

	if (planned_check > service->GetNextCheck()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Ignoring reschedule request for service '"
			<< arguments[1] << "' (next check is already sooner than requested check time)";
		return;
	}

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Rescheduling next check for service '" << arguments[1] << "'";

	if (planned_check < Utility::GetTime())
		planned_check = Utility::GetTime();

	service->SetNextCheck(planned_check);

	/* trigger update event for DB IDO */
	Checkable::OnNextCheckUpdated(service);
}

void ExternalCommandProcessor::ScheduleForcedSvcCheck(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot reschedule forced service check for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Rescheduling next check for service '" << arguments[1] << "'";

	service->SetForceNextCheck(true);
	service->SetNextCheck(Convert::ToDouble(arguments[2]));

	/* trigger update event for DB IDO */
	Checkable::OnNextCheckUpdated(service);
}

void ExternalCommandProcessor::EnableHostCheck(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable host checks for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling active checks for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_active_checks", true);
}

void ExternalCommandProcessor::DisableHostCheck(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable host check non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling active checks for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_active_checks", false);
}

void ExternalCommandProcessor::EnableSvcCheck(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable service check for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling active checks for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_active_checks", true);
}

void ExternalCommandProcessor::DisableSvcCheck(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable service check for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling active checks for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_active_checks", false);
}

void ExternalCommandProcessor::ShutdownProcess(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Shutting down Icinga via external command.");
	Application::RequestShutdown();
}

void ExternalCommandProcessor::RestartProcess(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Restarting Icinga via external command.");
	Application::RequestRestart();
}

void ExternalCommandProcessor::ScheduleForcedHostSvcChecks(double, const std::vector<String>& arguments)
{
	double planned_check = Convert::ToDouble(arguments[1]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot reschedule forced host service checks for non-existent host '" + arguments[0] + "'"));

	for (const Service::Ptr& service : host->GetServices()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Rescheduling next check for service '" << service->GetName() << "'";

		service->SetNextCheck(planned_check);
		service->SetForceNextCheck(true);

		/* trigger update event for DB IDO */
		Checkable::OnNextCheckUpdated(service);
	}
}

void ExternalCommandProcessor::ScheduleHostSvcChecks(double, const std::vector<String>& arguments)
{
	double planned_check = Convert::ToDouble(arguments[1]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot reschedule host service checks for non-existent host '" + arguments[0] + "'"));

	if (planned_check < Utility::GetTime())
		planned_check = Utility::GetTime();

	for (const Service::Ptr& service : host->GetServices()) {
		if (planned_check > service->GetNextCheck()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Ignoring reschedule request for service '"
				<< service->GetName() << "' (next check is already sooner than requested check time)";
			continue;
		}

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Rescheduling next check for service '" << service->GetName() << "'";

		service->SetNextCheck(planned_check);

		/* trigger update event for DB IDO */
		Checkable::OnNextCheckUpdated(service);
	}
}

void ExternalCommandProcessor::EnableHostSvcChecks(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable host service checks for non-existent host '" + arguments[0] + "'"));

	for (const Service::Ptr& service : host->GetServices()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling active checks for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_active_checks", true);
	}
}

void ExternalCommandProcessor::DisableHostSvcChecks(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable host service checks for non-existent host '" + arguments[0] + "'"));

	for (const Service::Ptr& service : host->GetServices()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling active checks for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_active_checks", false);
	}
}

void ExternalCommandProcessor::AcknowledgeSvcProblem(double, const std::vector<String>& arguments)
{
	bool sticky = (Convert::ToLong(arguments[2]) == 2 ? true : false);
	bool notify = (Convert::ToLong(arguments[3]) > 0 ? true : false);
	bool persistent = (Convert::ToLong(arguments[4]) > 0 ? true : false);

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot acknowledge service problem for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	if (service->GetState() == ServiceOK)
		BOOST_THROW_EXCEPTION(std::invalid_argument("The service '" + arguments[1] + "' is OK."));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Setting acknowledgement for service '" << service->GetName() << "'" << (notify ? "" : ". Disabled notification");

	Comment::AddComment(service, CommentAcknowledgement, arguments[5], arguments[6], persistent, 0);
	service->AcknowledgeProblem(arguments[5], arguments[6], sticky ? AcknowledgementSticky : AcknowledgementNormal, notify, persistent);
}

void ExternalCommandProcessor::AcknowledgeSvcProblemExpire(double, const std::vector<String>& arguments)
{
	bool sticky = (Convert::ToLong(arguments[2]) == 2 ? true : false);
	bool notify = (Convert::ToLong(arguments[3]) > 0 ? true : false);
	bool persistent = (Convert::ToLong(arguments[4]) > 0 ? true : false);
	double timestamp = Convert::ToDouble(arguments[5]);

	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot acknowledge service problem with expire time for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	if (service->GetState() == ServiceOK)
		BOOST_THROW_EXCEPTION(std::invalid_argument("The service '" + arguments[1] + "' is OK."));

	if (timestamp != 0 && timestamp <= Utility::GetTime())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Acknowledgement expire time must be in the future for service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Setting timed acknowledgement for service '" << service->GetName() << "'" << (notify ? "" : ". Disabled notification");

	Comment::AddComment(service, CommentAcknowledgement, arguments[6], arguments[7], persistent, timestamp);
	service->AcknowledgeProblem(arguments[6], arguments[7], sticky ? AcknowledgementSticky : AcknowledgementNormal, notify, persistent, timestamp);
}

void ExternalCommandProcessor::RemoveSvcAcknowledgement(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot remove service acknowledgement for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing acknowledgement for service '" << service->GetName() << "'";

	{
		ObjectLock olock(service);
		service->ClearAcknowledgement();
	}

	service->RemoveCommentsByType(CommentAcknowledgement);
}

void ExternalCommandProcessor::AcknowledgeHostProblem(double, const std::vector<String>& arguments)
{
	bool sticky = (Convert::ToLong(arguments[1]) == 2 ? true : false);
	bool notify = (Convert::ToLong(arguments[2]) > 0 ? true : false);
	bool persistent = (Convert::ToLong(arguments[3]) > 0 ? true : false);

	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot acknowledge host problem for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Setting acknowledgement for host '" << host->GetName() << "'" << (notify ? "" : ". Disabled notification");

	if (host->GetState() == HostUp)
		BOOST_THROW_EXCEPTION(std::invalid_argument("The host '" + arguments[0] + "' is OK."));

	Comment::AddComment(host, CommentAcknowledgement, arguments[4], arguments[5], persistent, 0);
	host->AcknowledgeProblem(arguments[4], arguments[5], sticky ? AcknowledgementSticky : AcknowledgementNormal, notify, persistent);
}

void ExternalCommandProcessor::AcknowledgeHostProblemExpire(double, const std::vector<String>& arguments)
{
	bool sticky = (Convert::ToLong(arguments[1]) == 2 ? true : false);
	bool notify = (Convert::ToLong(arguments[2]) > 0 ? true : false);
	bool persistent = (Convert::ToLong(arguments[3]) > 0 ? true : false);
	double timestamp = Convert::ToDouble(arguments[4]);

	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot acknowledge host problem with expire time for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Setting timed acknowledgement for host '" << host->GetName() << "'" << (notify ? "" : ". Disabled notification");

	if (host->GetState() == HostUp)
		BOOST_THROW_EXCEPTION(std::invalid_argument("The host '" + arguments[0] + "' is OK."));

	if (timestamp != 0 && timestamp <= Utility::GetTime())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Acknowledgement expire time must be in the future for host '" + arguments[0] + "'"));

	Comment::AddComment(host, CommentAcknowledgement, arguments[5], arguments[6], persistent, timestamp);
	host->AcknowledgeProblem(arguments[5], arguments[6], sticky ? AcknowledgementSticky : AcknowledgementNormal, notify, persistent, timestamp);
}

void ExternalCommandProcessor::RemoveHostAcknowledgement(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot remove acknowledgement for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing acknowledgement for host '" << host->GetName() << "'";

	{
		ObjectLock olock(host);
		host->ClearAcknowledgement();
	}
	host->RemoveCommentsByType(CommentAcknowledgement);
}

void ExternalCommandProcessor::EnableHostgroupSvcChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable hostgroup service checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Enabling active checks for service '" << service->GetName() << "'";

			service->ModifyAttribute("enable_active_checks", true);
		}
	}
}

void ExternalCommandProcessor::DisableHostgroupSvcChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable hostgroup service checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Disabling active checks for service '" << service->GetName() << "'";

			service->ModifyAttribute("enable_active_checks", false);
		}
	}
}

void ExternalCommandProcessor::EnableServicegroupSvcChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable servicegroup service checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling active checks for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_active_checks", true);
	}
}

void ExternalCommandProcessor::DisableServicegroupSvcChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable servicegroup service checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling active checks for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_active_checks", false);
	}
}

void ExternalCommandProcessor::EnablePassiveHostChecks(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable passive host checks for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling passive checks for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_passive_checks", true);
}

void ExternalCommandProcessor::DisablePassiveHostChecks(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable passive host checks for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling passive checks for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_passive_checks", false);
}

void ExternalCommandProcessor::EnablePassiveSvcChecks(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable service checks for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling passive checks for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_passive_checks", true);
}

void ExternalCommandProcessor::DisablePassiveSvcChecks(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable service checks for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling passive checks for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_passive_checks", false);
}

void ExternalCommandProcessor::EnableServicegroupPassiveSvcChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable servicegroup passive service checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling passive checks for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_passive_checks", true);
	}
}

void ExternalCommandProcessor::DisableServicegroupPassiveSvcChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable servicegroup passive service checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling passive checks for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_passive_checks", false);
	}
}

void ExternalCommandProcessor::EnableHostgroupPassiveSvcChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable hostgroup passive service checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Enabling passive checks for service '" << service->GetName() << "'";

			service->ModifyAttribute("enable_passive_checks", true);
		}
	}
}

void ExternalCommandProcessor::DisableHostgroupPassiveSvcChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable hostgroup passive service checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Disabling passive checks for service '" << service->GetName() << "'";

			service->ModifyAttribute("enable_passive_checks", false);
		}
	}
}

void ExternalCommandProcessor::ProcessFile(double, const std::vector<String>& arguments)
{
	std::deque< std::vector<String> > file_queue;
	file_queue.push_back(arguments);

	while (!file_queue.empty()) {
		std::vector<String> argument = file_queue.front();
		file_queue.pop_front();

		String file = argument[0];
		int to_delete = Convert::ToLong(argument[1]);

		std::ifstream ifp;
		ifp.exceptions(std::ifstream::badbit);

		ifp.open(file.CStr(), std::ifstream::in);

		while (ifp.good()) {
			std::string line;
			std::getline(ifp, line);

			try {
				Log(LogNotice, "compat")
					<< "Executing external command: " << line;

				ExecuteFromFile(line, file_queue);
			} catch (const std::exception& ex) {
				Log(LogWarning, "ExternalCommandProcessor")
					<< "External command failed: " << DiagnosticInformation(ex);
			}
		}

		ifp.close();

		if (to_delete > 0)
			(void) unlink(file.CStr());
	}
}

void ExternalCommandProcessor::ScheduleSvcDowntime(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule service downtime for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[5]);
	int is_fixed = Convert::ToLong(arguments[4]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating downtime for service " << service->GetName();
	(void) Downtime::AddDowntime(service, arguments[7], arguments[8],
		Convert::ToDouble(arguments[2]), Convert::ToDouble(arguments[3]),
		Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[6]));
}

void ExternalCommandProcessor::DelSvcDowntime(double, const std::vector<String>& arguments)
{
	int id = Convert::ToLong(arguments[0]);
	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing downtime ID " << arguments[0];
	String rid = Downtime::GetDowntimeIDFromLegacyID(id);
	Downtime::RemoveDowntime(rid, true);
}

void ExternalCommandProcessor::ScheduleHostDowntime(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule host downtime for non-existent host '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating downtime for host " << host->GetName();

	(void) Downtime::AddDowntime(host, arguments[6], arguments[7],
		Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
}

void ExternalCommandProcessor::ScheduleAndPropagateHostDowntime(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule and propagate host downtime for non-existent host '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating downtime for host " << host->GetName();

	(void) Downtime::AddDowntime(host, arguments[6], arguments[7],
		Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));

	/* Schedule downtime for all child hosts */
	for (const Checkable::Ptr& child : host->GetAllChildren()) {
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(child);

		/* ignore all service children */
		if (service)
			continue;

		(void) Downtime::AddDowntime(child, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleAndPropagateTriggeredHostDowntime(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule and propagate triggered host downtime for non-existent host '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating downtime for host " << host->GetName();

	String parentDowntime = Downtime::AddDowntime(host, arguments[6], arguments[7],
		Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));

	/* Schedule downtime for all child hosts and explicitely trigger them through the parent host's downtime */
	for (const Checkable::Ptr& child : host->GetAllChildren()) {
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(child);

		/* ignore all service children */
		if (service)
			continue;

		(void) Downtime::AddDowntime(child, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), parentDowntime, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::DelHostDowntime(double, const std::vector<String>& arguments)
{
	int id = Convert::ToLong(arguments[0]);
	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing downtime ID " << arguments[0];
	String rid = Downtime::GetDowntimeIDFromLegacyID(id);
	Downtime::RemoveDowntime(rid, true);
}

void ExternalCommandProcessor::DelDowntimeByHostName(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule host services downtime for non-existent host '" + arguments[0] + "'"));

	String serviceName;
	if (arguments.size() >= 2)
		serviceName = arguments[1];

	String startTime;
	if (arguments.size() >= 3)
		startTime = arguments[2];

	String commentString;
	if (arguments.size() >= 4)
		commentString = arguments[3];

	if (arguments.size() > 5)
		Log(LogWarning, "ExternalCommandProcessor")
			<< ("Ignoring additional parameters for host '" + arguments[0] + "' downtime deletion.");

	for (const Downtime::Ptr& downtime : host->GetDowntimes()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Removing downtime '" << downtime->GetName() << "'.";

		Downtime::RemoveDowntime(downtime->GetName(), true);
	}

	for (const Service::Ptr& service : host->GetServices()) {
		if (!serviceName.IsEmpty() && serviceName != service->GetName())
			continue;

		for (const Downtime::Ptr& downtime : service->GetDowntimes()) {
			if (!startTime.IsEmpty() && downtime->GetStartTime() != Convert::ToDouble(startTime))
				continue;

			if (!commentString.IsEmpty() && downtime->GetComment() != commentString)
				continue;

			Log(LogNotice, "ExternalCommandProcessor")
				<< "Removing downtime '" << downtime->GetName() << "'.";

			Downtime::RemoveDowntime(downtime->GetName(), true);
		}
	}
}

void ExternalCommandProcessor::ScheduleHostSvcDowntime(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule host services downtime for non-existent host '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating downtime for host " << host->GetName();

	(void) Downtime::AddDowntime(host, arguments[6], arguments[7],
		Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
		Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));

	for (const Service::Ptr& service : host->GetServices()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Creating downtime for service " << service->GetName();
		(void) Downtime::AddDowntime(service, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleHostgroupHostDowntime(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule hostgroup host downtime for non-existent hostgroup '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<<  "Creating downtime for host " << host->GetName();

		(void) Downtime::AddDowntime(host, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleHostgroupSvcDowntime(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule hostgroup service downtime for non-existent hostgroup '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	/* Note: we can't just directly create downtimes for all the services by iterating
	 * over all hosts in the host group - otherwise we might end up creating multiple
	 * downtimes for some services. */

	std::set<Service::Ptr> services;

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			services.insert(service);
		}
	}

	for (const Service::Ptr& service : services) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Creating downtime for service " << service->GetName();
		(void) Downtime::AddDowntime(service, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleServicegroupHostDowntime(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule servicegroup host downtime for non-existent servicegroup '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	/* Note: we can't just directly create downtimes for all the hosts by iterating
	 * over all services in the service group - otherwise we might end up creating multiple
	 * downtimes for some hosts. */

	std::set<Host::Ptr> hosts;

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();
		hosts.insert(host);
	}

	for (const Host::Ptr& host : hosts) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Creating downtime for host " << host->GetName();
		(void) Downtime::AddDowntime(host, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::ScheduleServicegroupSvcDowntime(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot schedule servicegroup service downtime for non-existent servicegroup '" + arguments[0] + "'"));

	String triggeredBy;
	int triggeredByLegacy = Convert::ToLong(arguments[4]);
	int is_fixed = Convert::ToLong(arguments[3]);
	if (triggeredByLegacy != 0)
		triggeredBy = Downtime::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Creating downtime for service " << service->GetName();
		(void) Downtime::AddDowntime(service, arguments[6], arguments[7],
			Convert::ToDouble(arguments[1]), Convert::ToDouble(arguments[2]),
			Convert::ToBool(is_fixed), triggeredBy, Convert::ToDouble(arguments[5]));
	}
}

void ExternalCommandProcessor::AddHostComment(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot add host comment for non-existent host '" + arguments[0] + "'"));

	if (arguments[2].IsEmpty() || arguments[3].IsEmpty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Author and comment must not be empty"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating comment for host " << host->GetName();
	(void) Comment::AddComment(host, CommentUser, arguments[2], arguments[3], false, 0);
}

void ExternalCommandProcessor::DelHostComment(double, const std::vector<String>& arguments)
{
	int id = Convert::ToLong(arguments[0]);
	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing comment ID " << arguments[0];
	String rid = Comment::GetCommentIDFromLegacyID(id);
	Comment::RemoveComment(rid);
}

void ExternalCommandProcessor::AddSvcComment(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot add service comment for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	if (arguments[3].IsEmpty() || arguments[4].IsEmpty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Author and comment must not be empty"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Creating comment for service " << service->GetName();
	(void) Comment::AddComment(service, CommentUser, arguments[3], arguments[4], false, 0);
}

void ExternalCommandProcessor::DelSvcComment(double, const std::vector<String>& arguments)
{
	int id = Convert::ToLong(arguments[0]);
	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing comment ID " << arguments[0];

	String rid = Comment::GetCommentIDFromLegacyID(id);
	Comment::RemoveComment(rid);
}

void ExternalCommandProcessor::DelAllHostComments(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot delete all host comments for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing all comments for host " << host->GetName();
	host->RemoveAllComments();
}

void ExternalCommandProcessor::DelAllSvcComments(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot delete all service comments for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Removing all comments for service " << service->GetName();
	service->RemoveAllComments();
}

void ExternalCommandProcessor::SendCustomHostNotification(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot send custom host notification for non-existent host '" + arguments[0] + "'"));

	int options = Convert::ToLong(arguments[1]);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Sending custom notification for host " << host->GetName();
	if (options & 2) {
		host->SetForceNextNotification(true);
	}

	Checkable::OnNotificationsRequested(host, NotificationCustom,
		host->GetLastCheckResult(), arguments[2], arguments[3], nullptr);
}

void ExternalCommandProcessor::SendCustomSvcNotification(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot send custom service notification for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	int options = Convert::ToLong(arguments[2]);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Sending custom notification for service " << service->GetName();

	if (options & 2) {
		service->SetForceNextNotification(true);
	}

	Service::OnNotificationsRequested(service, NotificationCustom,
		service->GetLastCheckResult(), arguments[3], arguments[4], nullptr);
}

void ExternalCommandProcessor::DelayHostNotification(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot delay host notification for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Delaying notifications for host '" << host->GetName() << "'";

	for (const Notification::Ptr& notification : host->GetNotifications()) {
		notification->SetNextNotification(Convert::ToDouble(arguments[1]));
	}
}

void ExternalCommandProcessor::DelaySvcNotification(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot delay service notification for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Delaying notifications for service " << service->GetName();

	for (const Notification::Ptr& notification : service->GetNotifications()) {
		notification->SetNextNotification(Convert::ToDouble(arguments[2]));
	}
}

void ExternalCommandProcessor::EnableHostNotifications(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable host notifications for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling notifications for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_notifications", true);
}

void ExternalCommandProcessor::DisableHostNotifications(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable host notifications for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling notifications for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_notifications", false);
}

void ExternalCommandProcessor::EnableSvcNotifications(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable service notifications for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling notifications for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_notifications", true);
}

void ExternalCommandProcessor::DisableSvcNotifications(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable service notifications for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling notifications for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_notifications", false);
}

void ExternalCommandProcessor::EnableHostSvcNotifications(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable notifications for all services for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling notifications for all services on host '" << arguments[0] << "'";

	for (const Service::Ptr& service : host->GetServices()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling notifications for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_notifications", true);
	}
}

void ExternalCommandProcessor::DisableHostSvcNotifications(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable notifications for all services  for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling notifications for all services on host '" << arguments[0] << "'";

	for (const Service::Ptr& service : host->GetServices()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling notifications for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_notifications", false);
	}
}

void ExternalCommandProcessor::DisableHostgroupHostChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable hostgroup host checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling active checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_active_checks", false);
	}
}

void ExternalCommandProcessor::DisableHostgroupPassiveHostChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable hostgroup passive host checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling passive checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_passive_checks", false);
	}
}

void ExternalCommandProcessor::DisableServicegroupHostChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable servicegroup host checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling active checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_active_checks", false);
	}
}

void ExternalCommandProcessor::DisableServicegroupPassiveHostChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable servicegroup passive host checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling passive checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_passive_checks", false);
	}
}

void ExternalCommandProcessor::EnableHostgroupHostChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable hostgroup host checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling active checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_active_checks", true);
	}
}

void ExternalCommandProcessor::EnableHostgroupPassiveHostChecks(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable hostgroup passive host checks for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling passive checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_passive_checks", true);
	}
}

void ExternalCommandProcessor::EnableServicegroupHostChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable servicegroup host checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling active checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_active_checks", true);
	}
}

void ExternalCommandProcessor::EnableServicegroupPassiveHostChecks(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable servicegroup passive host checks for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling passive checks for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_passive_checks", true);
	}
}

void ExternalCommandProcessor::EnableHostFlapping(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable host flapping for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling flapping detection for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_flapping", true);
}

void ExternalCommandProcessor::DisableHostFlapping(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable host flapping for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling flapping detection for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_flapping", false);
}

void ExternalCommandProcessor::EnableSvcFlapping(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable service flapping for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling flapping detection for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_flapping", true);
}

void ExternalCommandProcessor::DisableSvcFlapping(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable service flapping for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling flapping detection for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_flapping", false);
}

void ExternalCommandProcessor::EnableNotifications(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally enabling notifications.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_notifications", true);
}

void ExternalCommandProcessor::DisableNotifications(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally disabling notifications.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_notifications", false);
}

void ExternalCommandProcessor::EnableFlapDetection(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally enabling flap detection.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_flapping", true);
}

void ExternalCommandProcessor::DisableFlapDetection(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally disabling flap detection.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_flapping", false);
}

void ExternalCommandProcessor::EnableEventHandlers(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally enabling event handlers.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_event_handlers", true);
}

void ExternalCommandProcessor::DisableEventHandlers(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally disabling event handlers.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_event_handlers", false);
}

void ExternalCommandProcessor::EnablePerformanceData(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally enabling performance data processing.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_perfdata", true);
}

void ExternalCommandProcessor::DisablePerformanceData(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally disabling performance data processing.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_perfdata", false);
}

void ExternalCommandProcessor::StartExecutingSvcChecks(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally enabling service checks.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_service_checks", true);
}

void ExternalCommandProcessor::StopExecutingSvcChecks(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally disabling service checks.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_service_checks", false);
}

void ExternalCommandProcessor::StartExecutingHostChecks(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally enabling host checks.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_host_checks", true);
}

void ExternalCommandProcessor::StopExecutingHostChecks(double, const std::vector<String>&)
{
	Log(LogNotice, "ExternalCommandProcessor", "Globally disabling host checks.");

	IcingaApplication::GetInstance()->ModifyAttribute("enable_host_checks", false);
}

void ExternalCommandProcessor::ChangeNormalSvcCheckInterval(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot update check interval for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	double interval = Convert::ToDouble(arguments[2]);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Updating check interval for service '" << arguments[1] << "'";

	service->ModifyAttribute("check_interval", interval * 60);
}

void ExternalCommandProcessor::ChangeNormalHostCheckInterval(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot update check interval for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Updating check interval for host '" << arguments[0] << "'";

	double interval = Convert::ToDouble(arguments[1]);

	host->ModifyAttribute("check_interval", interval * 60);
}

void ExternalCommandProcessor::ChangeRetrySvcCheckInterval(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot update retry interval for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	double interval = Convert::ToDouble(arguments[2]);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Updating retry interval for service '" << arguments[1] << "'";

	service->ModifyAttribute("retry_interval", interval * 60);
}

void ExternalCommandProcessor::ChangeRetryHostCheckInterval(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot update retry interval for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Updating retry interval for host '" << arguments[0] << "'";

	double interval = Convert::ToDouble(arguments[1]);

	host->ModifyAttribute("retry_interval", interval * 60);
}

void ExternalCommandProcessor::EnableHostEventHandler(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable event handler for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling event handler for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_event_handler", true);
}

void ExternalCommandProcessor::DisableHostEventHandler(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable event handler for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling event handler for host '" << arguments[0] << "'";

	host->ModifyAttribute("enable_event_handler", false);
}

void ExternalCommandProcessor::EnableSvcEventHandler(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable event handler for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Enabling event handler for service '" << arguments[1] << "'";

	service->ModifyAttribute("enable_event_handler", true);
}

void ExternalCommandProcessor::DisableSvcEventHandler(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable event handler for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Disabling event handler for service '" << arguments[1] + "'";

	service->ModifyAttribute("enable_event_handler", false);
}

void ExternalCommandProcessor::ChangeHostEventHandler(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change event handler for non-existent host '" + arguments[0] + "'"));

	if (arguments[1].IsEmpty()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Unsetting event handler for host '" << arguments[0] << "'";

		host->ModifyAttribute("event_command", "");
	} else {
		EventCommand::Ptr command = EventCommand::GetByName(arguments[1]);

		if (!command)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Event command '" + arguments[1] + "' does not exist."));

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Changing event handler for host '" << arguments[0] << "' to '" << arguments[1] << "'";

		host->ModifyAttribute("event_command", command->GetName());
	}
}

void ExternalCommandProcessor::ChangeSvcEventHandler(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change event handler for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	if (arguments[2].IsEmpty()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Unsetting event handler for service '" << arguments[1] << "'";

		service->ModifyAttribute("event_command", "");
	} else {
		EventCommand::Ptr command = EventCommand::GetByName(arguments[2]);

		if (!command)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Event command '" + arguments[2] + "' does not exist."));

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Changing event handler for service '" << arguments[1] << "' to '" << arguments[2] << "'";

		service->ModifyAttribute("event_command", command->GetName());
	}
}

void ExternalCommandProcessor::ChangeHostCheckCommand(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change check command for non-existent host '" + arguments[0] + "'"));

	CheckCommand::Ptr command = CheckCommand::GetByName(arguments[1]);

	if (!command)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Check command '" + arguments[1] + "' does not exist."));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing check command for host '" << arguments[0] << "' to '" << arguments[1] << "'";

	host->ModifyAttribute("check_command", command->GetName());
}

void ExternalCommandProcessor::ChangeSvcCheckCommand(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change check command for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	CheckCommand::Ptr command = CheckCommand::GetByName(arguments[2]);

	if (!command)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Check command '" + arguments[2] + "' does not exist."));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing check command for service '" << arguments[1] << "' to '" << arguments[2] << "'";

	service->ModifyAttribute("check_command", command->GetName());
}

void ExternalCommandProcessor::ChangeMaxHostCheckAttempts(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change max check attempts for non-existent host '" + arguments[0] + "'"));

	int attempts = Convert::ToLong(arguments[1]);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing max check attempts for host '" << arguments[0] << "' to '" << arguments[1] << "'";

	host->ModifyAttribute("max_check_attempts", attempts);
}

void ExternalCommandProcessor::ChangeMaxSvcCheckAttempts(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change max check attempts for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	int attempts = Convert::ToLong(arguments[2]);

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing max check attempts for service '" << arguments[1] << "' to '" << arguments[2] << "'";

	service->ModifyAttribute("max_check_attempts", attempts);
}

void ExternalCommandProcessor::ChangeHostCheckTimeperiod(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change check period for non-existent host '" + arguments[0] + "'"));

	TimePeriod::Ptr tp = TimePeriod::GetByName(arguments[1]);

	if (!tp)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Time period '" + arguments[1] + "' does not exist."));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing check period for host '" << arguments[0] << "' to '" << arguments[1] << "'";

	host->ModifyAttribute("check_period", tp->GetName());
}

void ExternalCommandProcessor::ChangeSvcCheckTimeperiod(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change check period for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	TimePeriod::Ptr tp = TimePeriod::GetByName(arguments[2]);

	if (!tp)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Time period '" + arguments[2] + "' does not exist."));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing check period for service '" << arguments[1] << "' to '" << arguments[2] << "'";

	service->ModifyAttribute("check_period", tp->GetName());
}

void ExternalCommandProcessor::ChangeCustomHostVar(double, const std::vector<String>& arguments)
{
	Host::Ptr host = Host::GetByName(arguments[0]);

	if (!host)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change custom var for non-existent host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing custom var '" << arguments[1] << "' for host '" << arguments[0] << "' to value '" << arguments[2] << "'";

	host->ModifyAttribute("vars." + arguments[1], arguments[2]);
}

void ExternalCommandProcessor::ChangeCustomSvcVar(double, const std::vector<String>& arguments)
{
	Service::Ptr service = Service::GetByNamePair(arguments[0], arguments[1]);

	if (!service)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change custom var for non-existent service '" + arguments[1] + "' on host '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing custom var '" << arguments[2] << "' for service '" << arguments[1] << "' on host '"
		<< arguments[0] << "' to value '" << arguments[3] << "'";

	service->ModifyAttribute("vars." + arguments[2], arguments[3]);
}

void ExternalCommandProcessor::ChangeCustomUserVar(double, const std::vector<String>& arguments)
{
	User::Ptr user = User::GetByName(arguments[0]);

	if (!user)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change custom var for non-existent user '" + arguments[0] + "'"));

	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing custom var '" << arguments[1] << "' for user '" << arguments[0] << "' to value '" << arguments[2] << "'";

	user->ModifyAttribute("vars." + arguments[1], arguments[2]);
}

void ExternalCommandProcessor::ChangeCustomCheckcommandVar(double, const std::vector<String>& arguments)
{
	CheckCommand::Ptr command = CheckCommand::GetByName(arguments[0]);

	if (!command)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change custom var for non-existent command '" + arguments[0] + "'"));

	ChangeCustomCommandVarInternal(command, arguments[1], arguments[2]);
}

void ExternalCommandProcessor::ChangeCustomEventcommandVar(double, const std::vector<String>& arguments)
{
	EventCommand::Ptr command = EventCommand::GetByName(arguments[0]);

	if (!command)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change custom var for non-existent command '" + arguments[0] + "'"));

	ChangeCustomCommandVarInternal(command, arguments[1], arguments[2]);
}

void ExternalCommandProcessor::ChangeCustomNotificationcommandVar(double, const std::vector<String>& arguments)
{
	NotificationCommand::Ptr command = NotificationCommand::GetByName(arguments[0]);

	if (!command)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot change custom var for non-existent command '" + arguments[0] + "'"));

	ChangeCustomCommandVarInternal(command, arguments[1], arguments[2]);
}

void ExternalCommandProcessor::ChangeCustomCommandVarInternal(const Command::Ptr& command, const String& name, const Value& value)
{
	Log(LogNotice, "ExternalCommandProcessor")
		<< "Changing custom var '" << name << "' for command '" << command->GetName() << "' to value '" << value << "'";

	command->ModifyAttribute("vars." + name, value);
}

void ExternalCommandProcessor::EnableHostgroupHostNotifications(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable host notifications for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling notifications for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_notifications", true);
	}
}

void ExternalCommandProcessor::EnableHostgroupSvcNotifications(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable service notifications for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Enabling notifications for service '" << service->GetName() << "'";

			service->ModifyAttribute("enable_notifications", true);
		}
	}
}

void ExternalCommandProcessor::DisableHostgroupHostNotifications(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable host notifications for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling notifications for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_notifications", false);
	}
}

void ExternalCommandProcessor::DisableHostgroupSvcNotifications(double, const std::vector<String>& arguments)
{
	HostGroup::Ptr hg = HostGroup::GetByName(arguments[0]);

	if (!hg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable service notifications for non-existent hostgroup '" + arguments[0] + "'"));

	for (const Host::Ptr& host : hg->GetMembers()) {
		for (const Service::Ptr& service : host->GetServices()) {
			Log(LogNotice, "ExternalCommandProcessor")
				<< "Disabling notifications for service '" << service->GetName() << "'";

			service->ModifyAttribute("enable_notifications", false);
		}
	}
}

void ExternalCommandProcessor::EnableServicegroupHostNotifications(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable host notifications for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling notifications for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_notifications", true);
	}
}

void ExternalCommandProcessor::EnableServicegroupSvcNotifications(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot enable service notifications for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Enabling notifications for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_notifications", true);
	}
}

void ExternalCommandProcessor::DisableServicegroupHostNotifications(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable host notifications for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Host::Ptr host = service->GetHost();

		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling notifications for host '" << host->GetName() << "'";

		host->ModifyAttribute("enable_notifications", false);
	}
}

void ExternalCommandProcessor::DisableServicegroupSvcNotifications(double, const std::vector<String>& arguments)
{
	ServiceGroup::Ptr sg = ServiceGroup::GetByName(arguments[0]);

	if (!sg)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot disable service notifications for non-existent servicegroup '" + arguments[0] + "'"));

	for (const Service::Ptr& service : sg->GetMembers()) {
		Log(LogNotice, "ExternalCommandProcessor")
			<< "Disabling notifications for service '" << service->GetName() << "'";

		service->ModifyAttribute("enable_notifications", false);
	}
}

boost::mutex& ExternalCommandProcessor::GetMutex()
{
	static boost::mutex mtx;
	return mtx;
}

std::map<String, ExternalCommandInfo>& ExternalCommandProcessor::GetCommands()
{
	static std::map<String, ExternalCommandInfo> commands;
	return commands;
}

