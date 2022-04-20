/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include "remote/pkiutility.hpp"
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
	Dictionary::Ptr result = new Dictionary({
		{ "code", code },
		{ "status", status }
	});

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
		return ApiActions::CreateResult(400, "Parameter 'exit_status' is required.");

	int exitStatus = HttpUtility::GetLastParameter(params, "exit_status");

	ServiceState state;

	if (!service) {
		if (exitStatus == 0)
			state = ServiceOK;
		else if (exitStatus == 1)
			state = ServiceCritical;
		else
			return ApiActions::CreateResult(400, "Invalid 'exit_status' for Host "
				+ checkable->GetName() + ".");
	} else {
		state = PluginUtility::ExitStatusToState(exitStatus);
	}

	if (!params->Contains("plugin_output"))
		return ApiActions::CreateResult(400, "Parameter 'plugin_output' is required");

	CheckResult::Ptr cr = new CheckResult();
	cr->SetOutput(HttpUtility::GetLastParameter(params, "plugin_output"));
	cr->SetState(state);

	if (params->Contains("execution_start"))
		cr->SetExecutionStart(HttpUtility::GetLastParameter(params, "execution_start"));

	if (params->Contains("execution_end"))
		cr->SetExecutionEnd(HttpUtility::GetLastParameter(params, "execution_end"));

	cr->SetCheckSource(HttpUtility::GetLastParameter(params, "check_source"));

	Value perfData = params->Get("performance_data");

	/* Allow to pass a performance data string from Icinga Web 2 next to the new Array notation. */
	if (perfData.IsString())
		cr->SetPerformanceData(PluginUtility::SplitPerfdata(perfData));
	else
		cr->SetPerformanceData(perfData);

	cr->SetCommand(params->Get("check_command"));

	/* Mark this check result as passive. */
	cr->SetActive(false);

	/* Result TTL allows to overrule the next expected freshness check. */
	if (params->Contains("ttl"))
		cr->SetTtl(HttpUtility::GetLastParameter(params, "ttl"));

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
		return ApiActions::CreateResult(400, "Parameter 'author' is required.");

	if (!params->Contains("comment"))
		return ApiActions::CreateResult(400, "Parameter 'comment' is required.");

	if (Convert::ToBool(HttpUtility::GetLastParameter(params, "force")))
		checkable->SetForceNextNotification(true);

	Checkable::OnNotificationsRequested(checkable, NotificationCustom, checkable->GetLastCheckResult(),
		HttpUtility::GetLastParameter(params, "author"), HttpUtility::GetLastParameter(params, "comment"), nullptr);

	return ApiActions::CreateResult(200, "Successfully sent custom notification for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::DelayNotification(const ConfigObject::Ptr& object,
	const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot delay notifications for non-existent object");

	if (!params->Contains("timestamp"))
		return ApiActions::CreateResult(400, "A timestamp is required to delay notifications");

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
		return ApiActions::CreateResult(400, "Acknowledgements require author and comment.");

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

	ObjectLock oLock (checkable);

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

	if (checkable->IsAcknowledged()) {
		return ApiActions::CreateResult(409, (service ? "Service " : "Host ") + checkable->GetName() + " is already acknowledged.");
	}

	Comment::AddComment(checkable, CommentAcknowledgement, HttpUtility::GetLastParameter(params, "author"),
		HttpUtility::GetLastParameter(params, "comment"), persistent, timestamp);
	checkable->AcknowledgeProblem(HttpUtility::GetLastParameter(params, "author"),
		HttpUtility::GetLastParameter(params, "comment"), sticky, notify, persistent, Utility::GetTime(), timestamp);

	return ApiActions::CreateResult(200, "Successfully acknowledged problem for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::RemoveAcknowledgement(const ConfigObject::Ptr& object,
	const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404,
			"Cannot remove acknowledgement for non-existent checkable object "
			+ object->GetName() + ".");

	String removedBy (HttpUtility::GetLastParameter(params, "author"));

	checkable->ClearAcknowledgement(removedBy);
	checkable->RemoveCommentsByType(CommentAcknowledgement, removedBy);

	return ApiActions::CreateResult(200, "Successfully removed acknowledgement for object '" + checkable->GetName() + "'.");
}

Dictionary::Ptr ApiActions::AddComment(const ConfigObject::Ptr& object,
	const Dictionary::Ptr& params)
{
	Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Cannot add comment for non-existent object");

	if (!params->Contains("author") || !params->Contains("comment"))
		return ApiActions::CreateResult(400, "Comments require author and comment.");

	String commentName = Comment::AddComment(checkable, CommentUser,
		HttpUtility::GetLastParameter(params, "author"),
		HttpUtility::GetLastParameter(params, "comment"), false, 0);

	Comment::Ptr comment = Comment::GetByName(commentName);

	Dictionary::Ptr additional = new Dictionary({
		{ "name", commentName },
		{ "legacy_id", comment->GetLegacyId() }
	});

	return ApiActions::CreateResult(200, "Successfully added comment '"
		+ commentName + "' for object '" + checkable->GetName()
		+ "'.", additional);
}

Dictionary::Ptr ApiActions::RemoveComment(const ConfigObject::Ptr& object,
	const Dictionary::Ptr& params)
{
	auto author (HttpUtility::GetLastParameter(params, "author"));
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (checkable) {
		std::set<Comment::Ptr> comments = checkable->GetComments();

		for (const Comment::Ptr& comment : comments) {
			{
				ObjectLock oLock (comment);
				comment->SetRemovedBy(author);
			}

			Comment::RemoveComment(comment->GetName());
		}

		return ApiActions::CreateResult(200, "Successfully removed all comments for object '" + checkable->GetName() + "'.");
	}

	Comment::Ptr comment = static_pointer_cast<Comment>(object);

	if (!comment)
		return ApiActions::CreateResult(404, "Cannot remove non-existent comment object.");

	{
		ObjectLock oLock (comment);
		comment->SetRemovedBy(author);
	}

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

		return ApiActions::CreateResult(400, "Options 'start_time', 'end_time', 'author' and 'comment' are required");
	}

	bool fixed = true;
	if (params->Contains("fixed"))
		fixed = HttpUtility::GetLastParameter(params, "fixed");

	if (!fixed && !params->Contains("duration"))
		return ApiActions::CreateResult(400, "Option 'duration' is required for flexible downtime");

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

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	DowntimeChildOptions childOptions = DowntimeNoChildren;
	if (params->Contains("child_options")) {
		try {
			childOptions = Downtime::ChildOptionsFromValue(HttpUtility::GetLastParameter(params, "child_options"));
		} catch (const std::exception&) {
			return ApiActions::CreateResult(400, "Option 'child_options' provided an invalid value.");
		}
	}

	Downtime::Ptr downtime = Downtime::AddDowntime(checkable, author, comment, startTime, endTime,
		fixed, triggerName, duration);
	String downtimeName = downtime->GetName();

	Dictionary::Ptr additional = new Dictionary({
		{ "name", downtimeName },
		{ "legacy_id", downtime->GetLegacyId() }
	});

	/* Schedule downtime for all services for the host type. */
	bool allServices = false;

	if (params->Contains("all_services"))
		allServices = HttpUtility::GetLastParameter(params, "all_services");

	if (allServices && !service) {
		ArrayData serviceDowntimes;

		for (const Service::Ptr& hostService : host->GetServices()) {
			Log(LogNotice, "ApiActions")
				<< "Creating downtime for service " << hostService->GetName() << " on host " << host->GetName();

			Downtime::Ptr serviceDowntime = Downtime::AddDowntime(hostService, author, comment, startTime, endTime,
				fixed, triggerName, duration);
			String serviceDowntimeName = serviceDowntime->GetName();

			serviceDowntimes.push_back(new Dictionary({
				{ "name", serviceDowntimeName },
				{ "legacy_id", serviceDowntime->GetLegacyId() }
			}));
		}

		additional->Set("service_downtimes", new Array(std::move(serviceDowntimes)));
	}

	/* Schedule downtime for all child objects. */
	if (childOptions != DowntimeNoChildren) {
		/* 'DowntimeTriggeredChildren' schedules child downtimes triggered by the parent downtime.
		 * 'DowntimeNonTriggeredChildren' schedules non-triggered downtimes for all children.
		 */
		if (childOptions == DowntimeTriggeredChildren)
			triggerName = downtimeName;

		Log(LogNotice, "ApiActions")
			<< "Processing child options " << childOptions << " for downtime " << downtimeName;

		ArrayData childDowntimes;

		std::set<Checkable::Ptr> allChildren = checkable->GetAllChildren();
		for (const Checkable::Ptr& child : allChildren) {
			Host::Ptr childHost;
			Service::Ptr childService;
			tie(childHost, childService) = GetHostService(child);

			if (allServices && childService &&
					allChildren.find(static_pointer_cast<Checkable>(childHost)) != allChildren.end()) {
				/* When scheduling downtimes for all service and all children, the current child is a service, and its
				 * host is also a child, skip it here. The downtime for this service will be scheduled below together
				 * with the downtimes of all services for that host. Scheduling it below ensures that the relation
				 * from the child service downtime to the child host downtime is set properly. */
				continue;
			}

			Log(LogNotice, "ApiActions")
				<< "Scheduling downtime for child object " << child->GetName();

			Downtime::Ptr childDowntime = Downtime::AddDowntime(child, author, comment, startTime, endTime,
				fixed, triggerName, duration);
			String childDowntimeName = childDowntime->GetName();

			Log(LogNotice, "ApiActions")
				<< "Add child downtime '" << childDowntimeName << "'.";

			Dictionary::Ptr childAdditional = new Dictionary({
				{ "name", childDowntimeName },
				{ "legacy_id", childDowntime->GetLegacyId() }
			});

			/* For a host, also schedule all service downtimes if requested. */
			if (allServices && !childService) {
				ArrayData childServiceDowntimes;

				for (const Service::Ptr& childService : childHost->GetServices()) {
					Log(LogNotice, "ApiActions")
						<< "Creating downtime for service " << childService->GetName() << " on child host " << childHost->GetName();

					Downtime::Ptr serviceDowntime = Downtime::AddDowntime(childService, author, comment, startTime, endTime,
						fixed, triggerName, duration);
					String serviceDowntimeName = serviceDowntime->GetName();

					childServiceDowntimes.push_back(new Dictionary({
						{ "name", serviceDowntimeName },
						{ "legacy_id", serviceDowntime->GetLegacyId() }
					}));
				}

				childAdditional->Set("service_downtimes", new Array(std::move(childServiceDowntimes)));
			}

			childDowntimes.push_back(childAdditional);
		}

		additional->Set("child_downtimes", new Array(std::move(childDowntimes)));
	}

	return ApiActions::CreateResult(200, "Successfully scheduled downtime '" +
		downtimeName + "' for object '" + checkable->GetName() + "'.", additional);
}

Dictionary::Ptr ApiActions::RemoveDowntime(const ConfigObject::Ptr& object,
	const Dictionary::Ptr& params)
{
	auto author (HttpUtility::GetLastParameter(params, "author"));
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (checkable) {
		std::set<Downtime::Ptr> downtimes = checkable->GetDowntimes();

		for (const Downtime::Ptr& downtime : downtimes) {
			{
				ObjectLock oLock (downtime);
				downtime->SetRemovedBy(author);
			}

			Downtime::RemoveDowntime(downtime->GetName(), true);
		}

		return ApiActions::CreateResult(200, "Successfully removed all downtimes for object '" + checkable->GetName() + "'.");
	}

	Downtime::Ptr downtime = static_pointer_cast<Downtime>(object);

	if (!downtime)
		return ApiActions::CreateResult(404, "Cannot remove non-existent downtime object.");

	{
		ObjectLock oLock (downtime);
		downtime->SetRemovedBy(author);
	}

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
		return ApiActions::CreateResult(400, "Option 'cn' is required");

	String cn = HttpUtility::GetLastParameter(params, "cn");

	ApiListener::Ptr listener = ApiListener::GetInstance();
	String salt = listener->GetTicketSalt();

	if (salt.IsEmpty())
		return ApiActions::CreateResult(500, "Ticket salt is not configured in ApiListener object");

	String ticket = PBKDF2_SHA1(cn, salt, 50000);

	Dictionary::Ptr additional = new Dictionary({
		{ "ticket", ticket }
	});

	return ApiActions::CreateResult(200, "Generated PKI ticket '" + ticket + "' for common name '"
		+ cn + "'.", additional);
}
