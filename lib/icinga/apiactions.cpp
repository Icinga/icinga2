/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "remote/apilistener.hpp"
#include "remote/httputility.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include <fstream>

using namespace icinga;

REGISTER_APIACTION(process_check_result, "Service;Host", &ApiActions::ProcessCheckResult);
REGISTER_APIACTION(reschedule_check, "Service;Host", &ApiActions::RescheduleCheck);
REGISTER_APIACTION(send_custom_notification, "Service;Host", &ApiActions::SendCustomNotification);
REGISTER_APIACTION(delay_notification, "Service;Host", &ApiActions::DelayNotification);
REGISTER_APIACTION(acknowledge_problem, "Service;Host", &ApiActions::AcknowledgeProblem);
REGISTER_APIACTION(remove_acknowledgement, "Service;Host", &ApiActions::RemoveAcknowledgement);
REGISTER_APIACTION(add_comment, "Service;Host", &ApiActions::AddComment);
REGISTER_APIACTION(remove_comment, "Service;Host;Comment", &ApiActions::RemoveComment);
REGISTER_APIACTION(schedule_downtime, "Service;Host", &ApiActions::ScheduleDowntime);
REGISTER_APIACTION(remove_downtime, "Service;Host;Downtime", &ApiActions::RemoveDowntime);
REGISTER_APIACTION(shutdown_process, "", &ApiActions::ShutdownProcess);
REGISTER_APIACTION(restart_process, "", &ApiActions::RestartProcess);
REGISTER_APIACTION(generate_ticket, "", &ApiActions::GenerateTicket);

Dictionary::Ptr ApiActions::CreateResult(int code, const String& status,
    const Dictionary::Ptr& additional)
{
	Dictionary::Ptr result = new Dictionary();
	result->Set("code", code);
	result->Set("status", status);

	if (additional)
		additional->CopyTo(result);

	return result;
}

Dictionary::Ptr ApiActions::ProcessCheckResult(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404,
		    "Cannot process passive check result for non-existent object.");

	if (!checkable->GetEnablePassiveChecks())
		return ApiActions::CreateResult(403, "Passive checks are disabled for object '" + checkable->GetName() + "'.");

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (!params->Contains("exit_status"))
		return ApiActions::CreateResult(403, "Parameter 'exit_status' is required.");

	int exitStatus = HttpUtility::GetLastParameter(params, "exit_status");

	ServiceState state;

	if (!service) {
		if (exitStatus == 0)
			state = ServiceOK;
		else if (exitStatus == 1)
			state = ServiceCritical;
		else
			return ApiActions::CreateResult(403, "Invalid 'exit_status' for Host "
			    + checkable->GetName() + ".");
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

	/* Mark this check result as passive. */
	cr->SetActive(false);

	checkable->ProcessCheckResult(cr);

	return ApiActions::CreateResult(200, "Successfully processed check result for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::RescheduleCheck(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot reschedule check for non-existent object.");

	if (Convert::ToBool(HttpUtility::GetLastParameter(params, "force")))
		checkable->SetForceNextCheck(true);

	double nextCheck;
	if (params->Contains("next_check"))
		nextCheck = HttpUtility::GetLastParameter(params, "next_check");
	else
		nextCheck = Utility::GetTime();

	checkable->SetNextCheck(nextCheck);

	/* trigger update event for DB IDO */
	Checkable::OnNextCheckUpdated(checkable);

	return ApiActions::CreateResult(200, "Successfully rescheduled check for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::SendCustomNotification(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot send notification for non-existent object.");

	if (!params->Contains("author"))
		return ApiActions::CreateResult(403, "Parameter 'author' is required.");

	if (!params->Contains("comment"))
		return ApiActions::CreateResult(403, "Parameter 'comment' is required.");

	if (Convert::ToBool(HttpUtility::GetLastParameter(params, "force")))
		checkable->SetForceNextNotification(true);

	Checkable::OnNotificationsRequested(checkable, NotificationCustom, checkable->GetLastCheckResult(),
	    HttpUtility::GetLastParameter(params, "author"), HttpUtility::GetLastParameter(params, "comment"), MessageOrigin::Ptr());

	return ApiActions::CreateResult(200, "Successfully sent custom notification for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::DelayNotification(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot delay notifications for non-existent object");

	if (!params->Contains("timestamp"))
		return ApiActions::CreateResult(403, "A timestamp is required to delay notifications");

	for (const Notification::Ptr& notification : checkable->GetNotifications()) {
		notification->SetNextNotification(HttpUtility::GetLastParameter(params, "timestamp"));
	}

	return ApiActions::CreateResult(200, "Successfully delayed notifications for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::AcknowledgeProblem(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot acknowledge problem for non-existent object.");

	if (!params->Contains("author") || !params->Contains("comment"))
		return ApiActions::CreateResult(403, "Acknowledgements require author and comment.");

	AcknowledgementType sticky = AcknowledgementNormal;
	bool notify = false;
	bool persistent = false;
	double timestamp = 0.0;

	if (params->Contains("sticky") && HttpUtility::GetLastParameter(params, "sticky"))
		sticky = AcknowledgementSticky;
	if (params->Contains("notify"))
		notify = HttpUtility::GetLastParameter(params, "notify");
	if (params->Contains("persistent"))
		persistent = HttpUtility::GetLastParameter(params, "persistent");
	if (params->Contains("expiry")) {
		timestamp = HttpUtility::GetLastParameter(params, "expiry");

		if (timestamp <= Utility::GetTime())
			return ApiActions::CreateResult(409, "Acknowledgement 'expiry' timestamp must be in the future for object " + checkable->GetName());
	} else
		timestamp = 0;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	if (!service) {
		if (host->GetState() == HostUp)
			return ApiActions::CreateResult(409, "Host " + checkable->GetName() + " is UP.");
	} else {
		if (service->GetState() == ServiceOK)
			return ApiActions::CreateResult(409, "Service " + checkable->GetName() + " is OK.");
	}

	Comment::AddComment(checkable, CommentAcknowledgement, HttpUtility::GetLastParameter(params, "author"),
	    HttpUtility::GetLastParameter(params, "comment"), persistent, timestamp);
	checkable->AcknowledgeProblem(HttpUtility::GetLastParameter(params, "author"),
	    HttpUtility::GetLastParameter(params, "comment"), sticky, notify, timestamp);

	return ApiActions::CreateResult(200, "Successfully acknowledged problem for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::RemoveAcknowledgement(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404,
		    "Cannot remove acknowlegement for non-existent checkable object "
		    + object->GetName() + ".");

	checkable->ClearAcknowledgement();
	checkable->RemoveCommentsByType(CommentAcknowledgement);

	return ApiActions::CreateResult(200, "Successfully removed acknowledgement for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::AddComment(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot add comment for non-existent object");

	if (!params->Contains("author") || !params->Contains("comment"))
		return ApiActions::CreateResult(403, "Comments require author and comment.");

	String commentName = Comment::AddComment(checkable, CommentUser,
	    HttpUtility::GetLastParameter(params, "author"),
	    HttpUtility::GetLastParameter(params, "comment"), false, 0);

	Comment::Ptr comment = Comment::GetByName(commentName);

	Dictionary::Ptr additional = new Dictionary();
	additional->Set("name", commentName);
	additional->Set("legacy_id", comment->GetLegacyId());

	return ApiActions::CreateResult(200, "Successfully added comment '"
	    + commentName + "' for object '" + checkable->GetName()
	    + "'.", additional);
}

Dictionary::Ptr ApiActions::RemoveComment(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (checkable) {
		std::set<Comment::Ptr> comments = checkable->GetComments();

		for (const Comment::Ptr& comment : comments) {
			Comment::RemoveComment(comment->GetName());
		}

		return ApiActions::CreateResult(200, "Successfully removed all comments for object '" + checkable->GetName() + "'.");
	}

	Comment::Ptr comment = static_pointer_cast<Comment>(object);

	if (!comment)
		return ApiActions::CreateResult(404, "Cannot remove non-existent comment object.");

	String commentName = comment->GetName();

	Comment::RemoveComment(commentName);

	return ApiActions::CreateResult(200, "Successfully removed comment '" + commentName + "'.");
}

Dictionary::Ptr ApiActions::ScheduleDowntime(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Can't schedule downtime for non-existent object.");

	if (!params->Contains("start_time") || !params->Contains("end_time") ||
	    !params->Contains("author") || !params->Contains("comment")) {

		return ApiActions::CreateResult(404, "Options 'start_time', 'end_time', 'author' and 'comment' are required");
	}

	bool fixed = true;
	if (params->Contains("fixed"))
		fixed = HttpUtility::GetLastParameter(params, "fixed");

	if (!fixed && !params->Contains("duration"))
		return ApiActions::CreateResult(404, "Option 'duration' is required for flexible downtime");

	double duration = 0.0;
	if (params->Contains("duration"))
		duration = HttpUtility::GetLastParameter(params, "duration");

	String triggerName;
	if (params->Contains("trigger_name"))
		triggerName = HttpUtility::GetLastParameter(params, "trigger_name");

	String author = HttpUtility::GetLastParameter(params, "author");
	String comment = HttpUtility::GetLastParameter(params, "comment");
	double startTime = HttpUtility::GetLastParameter(params, "start_time");
	double endTime = HttpUtility::GetLastParameter(params, "end_time");

	String downtimeName = Downtime::AddDowntime(checkable, author, comment, startTime, endTime,
	    fixed, triggerName, duration);

	Downtime::Ptr downtime = Downtime::GetByName(downtimeName);

	Dictionary::Ptr additional = new Dictionary();
	additional->Set("name", downtimeName);
	additional->Set("legacy_id", downtime->GetLegacyId());

	/* Schedule downtime for all child objects. */
	int childOptions = 0;
	if (params->Contains("child_options"))
		childOptions = HttpUtility::GetLastParameter(params, "child_options");

	if (childOptions > 0) {
		/* '1' schedules child downtimes triggered by the parent downtime.
		 * '2' schedules non-triggered downtimes for all children.
		 */
		if (childOptions == 1)
			triggerName = downtimeName;

		Array::Ptr childDowntimes = new Array();

		Log(LogCritical, "ApiActions")
		    << "Processing child options " << childOptions << " for downtime " << downtimeName;

		for (const Checkable::Ptr& child : checkable->GetAllChildren()) {
			Log(LogCritical, "ApiActions")
			   << "Scheduling downtime for child object " << child->GetName();

			String childDowntimeName = Downtime::AddDowntime(child, author, comment, startTime, endTime,
			    fixed, triggerName, duration);

			Log(LogCritical, "ApiActions")
			    << "Add child downtime '" << childDowntimeName << "'.";

			Downtime::Ptr childDowntime = Downtime::GetByName(childDowntimeName);

			Dictionary::Ptr additionalChild = new Dictionary();
			additionalChild->Set("name", childDowntimeName);
			additionalChild->Set("legacy_id", childDowntime->GetLegacyId());
			childDowntimes->Add(additionalChild);
		}

		additional->Set("child_downtimes", childDowntimes);
	}

	return ApiActions::CreateResult(200, "Successfully scheduled downtime '" +
	     downtimeName + "' for object '" + checkable->GetName() + "'.", additional);
}

Dictionary::Ptr ApiActions::RemoveDowntime(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (checkable) {
		std::set<Downtime::Ptr> downtimes = checkable->GetDowntimes();

		for (const Downtime::Ptr& downtime : downtimes) {
			Downtime::RemoveDowntime(downtime->GetName(), true);
		}

		return ApiActions::CreateResult(200, "Successfully removed all downtimes for object '" + checkable->GetName() + "'.");
	}

	Downtime::Ptr downtime = static_pointer_cast<Downtime>(object);

	if (!downtime)
		return ApiActions::CreateResult(404, "Cannot remove non-existent downtime object.");

	String downtimeName = downtime->GetName();

	Downtime::RemoveDowntime(downtimeName, true);

	return ApiActions::CreateResult(200, "Successfully removed downtime '" + downtimeName + "'.");
}

Dictionary::Ptr ApiActions::ShutdownProcess(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Application::RequestShutdown();

	return ApiActions::CreateResult(200, "Shutting down Icinga 2.");
}

Dictionary::Ptr ApiActions::RestartProcess(const ConfigObject::Ptr& object,
    const Dictionary::Ptr& params)
{
	Application::RequestRestart();

	return ApiActions::CreateResult(200, "Restarting Icinga 2.");
}

Dictionary::Ptr ApiActions::GenerateTicket(const ConfigObject::Ptr&,
    const Dictionary::Ptr& params)
{
	if (!params->Contains("cn"))
		return ApiActions::CreateResult(404, "Option 'cn' is required");

	String cn = HttpUtility::GetLastParameter(params, "cn");

	ApiListener::Ptr listener = ApiListener::GetInstance();
	String salt = listener->GetTicketSalt();

	if (salt.IsEmpty())
		return ApiActions::CreateResult(500, "Ticket salt is not configured in ApiListener object");

	String ticket = PBKDF2_SHA1(cn, salt, 50000);

	Dictionary::Ptr additional = new Dictionary();
	additional->Set("ticket", ticket);

	return ApiActions::CreateResult(200, "Generated PKI ticket '" + ticket + "' for common name '"
	    + cn + "'.", additional);
}
