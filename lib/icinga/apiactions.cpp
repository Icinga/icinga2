/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/apiactions.hpp"
#include "icinga/service.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "remote/apiaction.hpp"
#include "remote/httputility.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_APIACTION(process_check_result, "Service;Host", &ApiActions::ProcessCheckResult);
REGISTER_APIACTION(reschedule_check, "Service;Host", &ApiActions::RescheduleCheck);
REGISTER_APIACTION(delay_notifications, "Service;Host", &ApiActions::DelayNotifications); //TODO groups

REGISTER_APIACTION(acknowledge_problem, "Service;Host", &ApiActions::AcknowledgeProblem);
REGISTER_APIACTION(remove_acknowledgement, "Service;Host", &ApiActions::RemoveAcknowledgement);
REGISTER_APIACTION(add_comment, "Service;Host", &ApiActions::AddComment);
//REGISTER_APIACTION(remove_comment, "", &ApiActions::RemoveComment); TODO Actions without objects
REGISTER_APIACTION(remove_all_comments, "Service;Host", &ApiActions::RemoveAllComments);
REGISTER_APIACTION(schedule_downtime, "Service;Host", &ApiActions::ScheduleDowntime); //TODO groups
//REGISTER_APIACTION(remove_downtime, "Service;Host", &ApiActions::RemoveDowntime); //TODO groups

REGISTER_APIACTION(enable_passive_checks, "Service;Host", &ApiActions::EnablePassiveChecks); //TODO groups
REGISTER_APIACTION(disable_passive_checks, "Service;Host", &ApiActions::DisablePassiveChecks); //TODO groups
REGISTER_APIACTION(enable_active_checks, "Host", &ApiActions::EnableActiveChecks); //TODO groups
REGISTER_APIACTION(disable_active_checks, "Host", &ApiActions::DisableActiveChecks); //TODO groups
REGISTER_APIACTION(enable_notifications, "Service;Host", &ApiActions::EnableNotifications);
REGISTER_APIACTION(disable_notifications, "Service;Host", &ApiActions::DisableNotifications);
REGISTER_APIACTION(enable_flap_detection, "Service;Host", &ApiActions::EnableFlapDetection);
REGISTER_APIACTION(disable_flap_detection, "Service;Host", &ApiActions::DisableFlapDetection);

/*
REGISTER_APIACTION(change_event_handler, "Service;Host", &ApiActions::ChangeEventHandler);
REGISTER_APIACTION(change_check_command, "Service;Host", &ApiActions::ChangeCheckCommand);
REGISTER_APIACTION(change_max_check_attempts, "Service;Host", &ApiActions::ChangeMaxCheckAttempts);
REGISTER_APIACTION(change_check_period, "Service;Host", &ApiActions::ChangeCheckPeriod);
REGISTER_APIACTION(change_check_interval, "Service;Host", &ApiActions::ChangeCheckInterval);
REGISTER_APIACTION(change_retry_interval, "Service;Host", &ApiActions::ChangeRetryInterval);
*/
/*
REGISTER_APIACTION(enable_notifications, "", &ApiActions::EnableNotifications);
REGISTER_APIACTION(disable_notifications, "", &ApiActions::DisableNotifications);
REGISTER_APIACTION(enable_flap_detection, "", &ApiActions::EnableFlapDetection);
REGISTER_APIACTION(disable_flap_detection, "", &ApiActions::DisableFlapDetection);
REGISTER_APIACTION(enable_event_handlers, "", &ApiActions::EnableEventHandlers);
REGISTER_APIACTION(disable_event_handlers, "", &ApiActions::DisableEventHandlers);
REGISTER_APIACTION(enable_performance_data, "", &ApiActions::EnablePerformanceData);
REGISTER_APIACTION(disable_performance_data, "", &ApiActions::DisablePerformanceData);
REGISTER_APIACTION(start_executing_svc_checks, "", &ApiActions::StartExecutingSvcChecks);
REGISTER_APIACTION(stop_executing_svc_checks, "", &ApiActions::StopExecutingSvcChecks);
REGISTER_APIACTION(start_executing_host_checks, "", &ApiActions::StartExecutingHostChecks);
REGISTER_APIACTION(stop_executing_host_checks, "", &ApiActions::StopExecutingHostChecks);
*/

/*
REGISTER_APIACTION(shutdown_process, "", &ApiActions::ShutdownProcess);
REGISTER_APIACTION(restart_process, "", &ApiActions::RestartProcess);
REGISTER_APIACTION(process_file, "", &ApiActions::ProcessFile);
*/

Dictionary::Ptr ApiActions::CreateResult(int code, const String& status)
{
	Dictionary::Ptr result = new Dictionary();
	result->Set("code", code);
	result->Set("status", status);
	return result;
}

Dictionary::Ptr ApiActions::RescheduleCheck(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot reschedule check for non-existent object");

	if (Convert::ToBool(HttpUtility::GetLastParameter(params, "force")))
		checkable->SetForceNextCheck(true);

	double nextCheck;
	if (params->Contains("next_check"))
		nextCheck = HttpUtility::GetLastParameter(params, "next_check");
	else
		nextCheck = Utility::GetTime();

	checkable->SetNextCheck(nextCheck);

	return ApiActions::CreateResult(200, "Successfully rescheduled check for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::ProcessCheckResult(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot process passive check result for non-existent object");

	if (!checkable->GetEnablePassiveChecks())
		return ApiActions::CreateResult(403, "Passive checks are disabled for " + checkable->GetName());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (!params->Contains("exit_status"))
		return ApiActions::CreateResult(403, "Parameter 'exit_status' is required");

	int exitStatus = HttpUtility::GetLastParameter(params, "exit_status");

	ServiceState state;

	if (!service) {
		if (exitStatus == 0)
			state = ServiceOK;
		else if (exitStatus == 1)
			state = ServiceCritical;
		else
			return ApiActions::CreateResult(403, "Invalid 'exit_status' for Host " + checkable->GetName());
	} else {
		state = PluginUtility::ExitStatusToState(exitStatus);
	}

	if (!params->Contains("plugin_output"))
		return ApiActions::CreateResult(403, "Parameter 'plugin_output' is required");

	CheckResult::Ptr cr = new CheckResult();
	cr->SetOutput(HttpUtility::GetLastParameter(params, "plugin_output"));
	cr->SetState(state);

	cr->SetCheckSource(HttpUtility::GetLastParameter(params, "check_source"));
	cr->SetPerformanceData(params->Get("performance_data"));
	cr->SetCommand(params->Get("check_command"));
	cr->SetExecutionEnd(HttpUtility::GetLastParameter(params, "execution_end"));
	cr->SetExecutionStart(HttpUtility::GetLastParameter(params, "execution_start"));
	cr->SetScheduleEnd(HttpUtility::GetLastParameter(params, "schedule_end"));
	cr->SetScheduleStart(HttpUtility::GetLastParameter(params, "schedule_start"));

	checkable->ProcessCheckResult(cr);

	/* Reschedule the next check. The side effect of this is that for as long
	 * as we receive passive results for a service we won't execute any
	 * active checks. */
	checkable->SetNextCheck(Utility::GetTime() + checkable->GetCheckInterval());

	return ApiActions::CreateResult(200, "Successfully processed check result for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::EnablePassiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	//TODO check if group undso
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot enable passive checks for non-existent object");

	checkable->SetEnablePassiveChecks(true);

	return ApiActions::CreateResult(200, "Successfully enabled passive checks for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::DisablePassiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	//TODO check if group undso
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot disable passive checks non-existent object");

	checkable->SetEnablePassiveChecks(false);

	return ApiActions::CreateResult(200, "Successfully disabled passive checks for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::EnableActiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Host::Ptr host = static_pointer_cast<Host>(object);

	if (!host)
		return ApiActions::CreateResult(404, "Cannot enable checks for non-existent object");

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		service->SetEnableActiveChecks(true);
	}

	return ApiActions::CreateResult(200, "Successfully enabled active checks for " + host->GetName());
}

Dictionary::Ptr ApiActions::DisableActiveChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Host::Ptr host = static_pointer_cast<Host>(object);

	if (!host)
		return ApiActions::CreateResult(404, "Cannot enable checks for non-existent object");

	BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
		service->SetEnableActiveChecks(false);
	}

	return ApiActions::CreateResult(200, "Successfully disabled active checks for " + host->GetName());
}

Dictionary::Ptr ApiActions::AcknowledgeProblem(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot acknowledge problem for non-existent object");

	if (!params->Contains("author") || !params->Contains("comment"))
		return ApiActions::CreateResult(403, "Acknowledgements require an author and a comment");

	AcknowledgementType sticky = AcknowledgementNormal;
	bool notify = false;
	double timestamp = 0;
	if (params->Contains("sticky"))
		sticky = AcknowledgementSticky;
	if (params->Contains("notify"))
		notify = true;
	if (params->Contains("timestamp"))
		timestamp = HttpUtility::GetLastParameter(params, "timestamp");

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (!service) {
		if (host->GetState() == HostUp)
			return ApiActions::CreateResult(409, "Host " + checkable->GetName() + " is up");
	} else {
		if (service->GetState() == ServiceOK)
			return ApiActions::CreateResult(409, "Service " + checkable->GetName() + " is ok");
	}

	checkable->AddComment(CommentAcknowledgement, HttpUtility::GetLastParameter(params, "author"),
	    HttpUtility::GetLastParameter(params, "comment"), timestamp);
	checkable->AcknowledgeProblem(HttpUtility::GetLastParameter(params, "author"),
	    HttpUtility::GetLastParameter(params, "comment"), sticky, notify, timestamp);
	return ApiActions::CreateResult(200, "Successfully acknowledged problem for " +  checkable->GetName());
}

Dictionary::Ptr ApiActions::RemoveAcknowledgement(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot remove acknowlegement for non-existent object");

	checkable->ClearAcknowledgement();
	checkable->RemoveCommentsByType(CommentAcknowledgement);

	return ApiActions::CreateResult(200, "Successfully removed acknowledgement for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::AddComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot add comment for non-existent object");

	if (!params->Contains("author") || !params->Contains("comment"))
		return ApiActions::CreateResult(403, "Comments require an author and a comment");

	//TODO	Fragnen warum es (void) checkable->AddComment(..) war
	checkable->AddComment(CommentUser, HttpUtility::GetLastParameter(params, "author"),
	    HttpUtility::GetLastParameter(params, "comment"), 0);

	return ApiActions::CreateResult(200, "Successfully added comment for " + checkable->GetName());
}

/*
Dictionary::Ptr ApiActions::RemoveComment(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	if (!params->Contains("comment_id"))
		return ApiActions::CreateResult(403, "Comment removal requires an comment_id";

	int comment_id = HttpUtility::GetLastParameter(params, "comment_id");

	String rid = Service::GetCommentIDFromLegacyID(comment_id);
	Service::RemoveComment(rid);

	return ApiActions::CreateResult(200, "Successfully removed comment " + comment_id);
}
*/

Dictionary::Ptr ApiActions::RemoveAllComments(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot remove comments from non-existent object");

	checkable->RemoveAllComments();
	return ApiActions::CreateResult(200, "Successfully removed all comments for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::EnableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot enable notifications for non-existent object");

	checkable->SetEnableNotifications(true);
	return ApiActions::CreateResult(200, "Successfully enabled notifications for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::DisableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot disable notifications for non-existent object");

	checkable->SetEnableNotifications(true);
	return ApiActions::CreateResult(200, "Successfully disabled notifications for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::DelayNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot delay notifications for non-existent object");

	if (!params->Contains("timestamp"))
		return ApiActions::CreateResult(403, "A timestamp is required to delay notifications");

	BOOST_FOREACH(const Notification::Ptr& notification, checkable->GetNotifications()) {
		notification->SetNextNotification(HttpUtility::GetLastParameter(params, "timestamp"));
	}

	return ApiActions::CreateResult(200, "Successfully delayed notifications for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::ScheduleDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Can't schedule downtime for non-existent object");

	if (!params->Contains("start_time") || !params->Contains("end_time") || !params->Contains("duration") ||
		!params->Contains("author") || !params->Contains("comment"))
		return ApiActions::CreateResult(404, "Options 'start_time', 'end_time', 'duration', 'author' and 'comment' are required");
	//Duration from end_time - start_time ?

	bool fixd = false;
	if (params->Contains("fixed"))
		fixd = HttpUtility::GetLastParameter(params, "fixed");

	int triggeredByLegacy = params->Contains("trigger_id") ? (int) HttpUtility::GetLastParameter(params, "trigger_id") : 0;
	String triggeredBy;
	if (triggeredByLegacy)
		triggeredBy = Service::GetDowntimeIDFromLegacyID(triggeredByLegacy);

	checkable->AddDowntime(HttpUtility::GetLastParameter(params, "author"), HttpUtility::GetLastParameter(params, "comment"),
	    HttpUtility::GetLastParameter(params, "start_time"), HttpUtility::GetLastParameter(params, "end_time"),
	    fixd, triggeredBy, HttpUtility::GetLastParameter(params, "duration"));

	return ApiActions::CreateResult(200, "Successfully scheduled downtime for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::EnableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Can't enable flap detection for non-existent object");

	checkable->SetEnableFlapping(true);

	return ApiActions::CreateResult(200, "Successfully enabled flap detection for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::DisableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Can't disable flap detection for non-existent object");

	checkable->SetEnableFlapping(false);

	return ApiActions::CreateResult(200, "Successfully disabled flap detection for " + checkable->GetName());
}

/*
Dictionary::Ptr ApiActions::RemoveDowntime(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	if (!params->Contains("downtime_id"))
		return ApiActions::CreateResult(403, "Downtime removal requires a downtime_id";

	int downtime_id = HttpUtility::GetLastParameter(params, "downtime_id");

	String rid = Service::GetDowntimeIDFromLegacyID(downtime_id);
	Service::RemoveDowntime(rid);

	return ApiActions::CreateResult(200, "Successfully removed downtime " + downtime_id);
}
*/
/*
Dictionary::Ptr ApiActions::EnableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableNotifications(true);

	ApiActions::CreateResult(200, "Globally enabled notifications.");
}

Dictionary::Ptr ApiActions::DisableNotifications(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableNotifications(false);

	ApiActions::CreateResult(200, "Globally disabled notifications.");
}

Dictionary::Ptr ApiActions::EnableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableFlapping(true);

	ApiActions::CreateResult(200, "Globally enabled flap detection.");
}

Dictionary::Ptr ApiActions::DisableFlapDetection(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableFlapping(false);

	ApiActions::CreateResult(200, "Globally disabled flap detection.");
}

Dictionary::Ptr ApiActions::EnableEventHandlers(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableEventHandlers(true);

	ApiActions::CreateResult(200, "Globally enabled event handlers.");
}

Dictionary::Ptr ApiActions::DisableEventHandlers(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableEventHandlers(false);

	ApiActions::CreateResult(200, "Globally disabled event handlers.");
}

Dictionary::Ptr ApiActions::EnablePerformanceData(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnablePerfdata(true);

	ApiActions::CreateResult(200, "Globally enabled performance data processing.");
}

Dictionary::Ptr ApiActions::DisablePerformanceData(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnablePerfdata(false);

	ApiActions::CreateResult(200, "Globally disabled performance data processing.");
}

Dictionary::Ptr ApiActions::StartExecutingSvcChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableServiceChecks(true);

	ApiActions::CreateResult(200, "Globally enabled service checks.");
}

Dictionary::Ptr ApiActions::StopExecutingSvcChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableServiceChecks(false);

	ApiActions::CreateResult(200, "Globally disabled service checks.");
}

Dictionary::Ptr ApiActions::StartExecutingHostChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableHostChecks(true);

	ApiActions::CreateResult(200, "Globally enabled host checks.");
}

Dictionary::Ptr ApiActions::StopExecutingHostChecks(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	IcingaApplication::GetInstance()->SetEnableHostChecks(false);

	ApiActions::CreateResult(200, "Globally disabled host checks.");
}
*/

/*
Dictionary::Ptr ApiActions::ChangeEventHandler(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot change event handler of a non-existent object");

   // empty command string implicitely disables event handler 
	if (!params->Contains("event_command_name")) {
		checkable->SetEnableEventHandler(false);
		return ApiActions::CreateResult(200, "Successfully disabled event handler for " + checkable->GetName());
	} else {
		String event_name = HttpUtility::GetLastParameter(params, "event_command_name");

		EventCommand::Ptr command = EventCommand::GetByName(event_name);

		if (!command)
			return ApiActions::CreateResult(404, "Event command '" + event_name + "' does not exist");

		checkable->SetEventCommand(command);

		return ApiActions::CreateResult(200, "Successfully changed event command for " + checkable->GetName());
	}
}

Dictionary::Ptr ApiActions::ChangeCheckCommand(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot change check command of a non-existent object");
	if (!params->Contains("check_command_name"))
		return ApiActions::CreateResult(403, "Parameter 'check_command_name' is required");

	String check_name = HttpUtility::GetLastParameter(params, "check_command_name");

	CheckCommand::Ptr command = CheckCommand::GetByName(check_name);

	if (!command)
		return ApiActions::CreateResult(404, "Check command '" + check_name + "' does not exist");

	checkable->SetCheckCommand(command);

	return ApiActions::CreateResult(200, "Successfully changed check command for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::ChangeMaxCheckAttempts(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot change check command of a non-existent object");
	if (!params->Contains("max_check_attempts"))
		return ApiActions::CreateResult(403, "Parameter 'max_check_attempts' is required");

	checkable->SetMaxCheckAttempts(HttpUtility::GetLastParameter(params, "max_check_attempts"));

	return ApiActions::CreateResult(200, "Successfully changed the maximum check attempts for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::ChangeCheckPeriod(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot change check command of a non-existent object");
	if (!params->Contains("time_period_name"))
		return ApiActions::CreateResult(403, "Parameter 'time_period_name' is required");

	String time_period_name = HttpUtility::GetLastParameter(params, "time_period_name");

	TimePeriod::Ptr time_period = TimePeriod::GetByName(time_period_name);

	if (!time_period)
		return ApiActions::CreateResult(404, "Time period '" + time_period_name + "' does not exist");

	checkable->SetCheckPeriod(time_period);

	return ApiActions::CreateResult(200, "Successfully changed the time period for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::ChangeCheckInterval(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot change check command of a non-existent object");
	if (!params->Contains("check_interval"))
		return ApiActions::CreateResult(403, "Parameter 'check_interval' is required");

	checkable->SetCheckInterval(HttpUtility::GetLastParameter(params, "check_interval"));

	return ApiActions::CreateResult(200, "Successfully changed the check interval for " + checkable->GetName());
}

Dictionary::Ptr ApiActions::ChangeRetryInterval(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot change check command of a non-existent object");
	if (!params->Contains("retry_interval"))
		return ApiActions::CreateResult(403, "Parameter 'retry_interval' is required");

	checkable->SetRetryInterval(HttpUtility::GetLastParameter(params, "retry_interval"));

	return ApiActions::CreateResult(200, "Successfully changed the retry interval for " + checkable->GetName());
}
*/

/*
Dictionary::Ptr ApiActions::RestartProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Application::RequestShutdown();

	return ApiActions::CreateResult(200, "I don't exist!");
}

Dictionary::Ptr ApiActions::RestartProcess(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	Application::RequestRestart();

	return ApiActions::CreateResult(200, "That's not how this works");
}

Dictionary::Ptr ApiActions::ProcessFile(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	if (!params->Contains("file_name")
		return ApiActions::CreateResult(403, "Parameter 'file_name' is required");

	String file = HttpUtility::GetLastParameter(params, "file_name")

	bool del = true;
	if (!params->Contains("delete") || !HttpUtility::GetLastParameter(params, "delete"))
		del = false;

	std::ifstream ifp;
	ifp.exceptions(std::ifstream::badbit);

	ifp.open(file.CStr(), std::ifstream::in);

	while (ifp.good()) {
		std::string line;
		std::getline(ifp, line);

		try {
			Execute(line);
		} catch (const std::exception& ex) {
			ifp.close();
			return ApiActions::CreateResult(500, "Command execution failed");
		}
	}

	ifp.close();

	if (del)
		(void) unlink(file.CStr());

	return ApiActions::CreateResult(200, "Successfully processed " + (del?"and deleted ":"") + "file " + file);
}
*/
