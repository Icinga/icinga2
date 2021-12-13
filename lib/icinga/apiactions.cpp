/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/apiactions.hpp"
#include "icinga/checkable.hpp"
#include "icinga/service.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/clusterevents.hpp"
#include "remote/apiaction.hpp"
#include "remote/apilistener.hpp"
#include "remote/filterutility.hpp"
#include "remote/pkiutility.hpp"
#include "remote/httputility.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/defer.hpp"
#include "remote/actionshandler.hpp"
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
REGISTER_APIACTION(execute_command, "Service;Host", &ApiActions::ExecuteCommand);

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

	if (!checkable->IsReachable(DependencyCheckExecution))
		return ApiActions::CreateResult(200, "Ignoring passive check result for unreachable object '" + checkable->GetName() + "'.");

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
	cr->SetSchedulingSource(HttpUtility::GetLastParameter(params, "scheduling_source"));

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

	double timestamp = 0.0;

	if (params->Contains("expiry")) {
		timestamp = HttpUtility::GetLastParameter(params, "expiry");
	}

	String commentName = Comment::AddComment(checkable, CommentUser,
		HttpUtility::GetLastParameter(params, "author"),
		HttpUtility::GetLastParameter(params, "comment"), false, timestamp);

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
			Comment::RemoveComment(comment->GetName(), true, author);
		}

		return ApiActions::CreateResult(200, "Successfully removed all comments for object '" + checkable->GetName() + "'.");
	}

	Comment::Ptr comment = static_pointer_cast<Comment>(object);

	if (!comment)
		return ApiActions::CreateResult(404, "Cannot remove non-existent comment object.");

	String commentName = comment->GetName();

	Comment::RemoveComment(commentName, true, author);

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
				fixed, triggerName, duration, String(), String(), downtimeName);
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
						fixed, triggerName, duration, String(), String(), childDowntimeName);
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

	size_t childCount = 0;

	if (checkable) {
		std::set<Downtime::Ptr> downtimes = checkable->GetDowntimes();

		for (const Downtime::Ptr& downtime : downtimes) {
			childCount += downtime->GetChildren().size();

			try {
				Downtime::RemoveDowntime(downtime->GetName(), true, true, false, author);
			} catch (const invalid_downtime_removal_error& error) {
				Log(LogWarning, "ApiActions") << error.what();

				return ApiActions::CreateResult(400, error.what());
			}
		}

		return ApiActions::CreateResult(200, "Successfully removed all downtimes for object '" +
			checkable->GetName() + "' and " + std::to_string(childCount) + " child downtimes.");
	}

	Downtime::Ptr downtime = static_pointer_cast<Downtime>(object);

	if (!downtime)
		return ApiActions::CreateResult(404, "Cannot remove non-existent downtime object.");

	childCount += downtime->GetChildren().size();

	try {
		String downtimeName = downtime->GetName();
		Downtime::RemoveDowntime(downtimeName, true, true, false, author);

		return ApiActions::CreateResult(200, "Successfully removed downtime '" + downtimeName +
			"' and " + std::to_string(childCount) + " child downtimes.");
	} catch (const invalid_downtime_removal_error& error) {
		Log(LogWarning, "ApiActions") << error.what();

		return ApiActions::CreateResult(400, error.what());
	}
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

Value ApiActions::GetSingleObjectByNameUsingPermissions(const String& type, const String& objectName, const ApiUser::Ptr& user)
{
	Dictionary::Ptr queryParams = new Dictionary();
	queryParams->Set("type", type);
	queryParams->Set(type.ToLower(), objectName);

	QueryDescription qd;
	qd.Types.insert(type);
	qd.Permission = "objects/query/" + type;

	std::vector<Value> objs;

	try {
		objs = FilterUtility::GetFilterTargets(qd, queryParams, user);
	} catch (const std::exception& ex) {
		Log(LogWarning, "ApiActions") << DiagnosticInformation(ex);
		return nullptr;
	}

	if (objs.empty())
		return nullptr;

	return objs.at(0);
};

Dictionary::Ptr ApiActions::ExecuteCommand(const ConfigObject::Ptr& object, const Dictionary::Ptr& params)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		BOOST_THROW_EXCEPTION(std::invalid_argument("No ApiListener instance configured."));

	/* Get command_type */
	String command_type = "EventCommand";

	if (params->Contains("command_type"))
		command_type = HttpUtility::GetLastParameter(params, "command_type");

	/* Validate command_type */
	if (command_type != "EventCommand" && command_type != "CheckCommand" && command_type != "NotificationCommand")
		return ApiActions::CreateResult(400, "Invalid command_type '" + command_type + "'.");

	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (!checkable)
		return ApiActions::CreateResult(404, "Can't start a command execution for a non-existent object.");

	/* Get TTL param */
	if (!params->Contains("ttl"))
		return ApiActions::CreateResult(400, "Parameter ttl is required.");

	double ttl = HttpUtility::GetLastParameter(params, "ttl");

	if (ttl <= 0)
		return ApiActions::CreateResult(400, "Parameter ttl must be greater than 0.");

	ObjectLock oLock (checkable);

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String endpoint = "$command_endpoint$";

	if (params->Contains("endpoint"))
		endpoint = HttpUtility::GetLastParameter(params, "endpoint");

	MacroProcessor::ResolverList resolvers;
	Value macros;

	if (params->Contains("macros")) {
		macros = HttpUtility::GetLastParameter(params, "macros");
		if (macros.IsObjectType<Dictionary>()) {
			resolvers.emplace_back("override", macros);
		} else {
			return ApiActions::CreateResult(400, "Parameter macros must be a dictionary.");
		}
	}

	if (service)
		resolvers.emplace_back("service", service);

	resolvers.emplace_back("host", host);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String resolved_endpoint = MacroProcessor::ResolveMacros(
		endpoint, resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), nullptr, false
	);

	if (!ActionsHandler::AuthenticatedApiUser)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Can't find API user."));

	/* Get endpoint */
	Endpoint::Ptr endpointPtr = GetSingleObjectByNameUsingPermissions(Endpoint::GetTypeName(), resolved_endpoint, ActionsHandler::AuthenticatedApiUser);

	if (!endpointPtr)
		return ApiActions::CreateResult(404, "Can't find a valid endpoint for '" + resolved_endpoint + "'.");

	/* Get command */
	String command;

	if (!params->Contains("command")) {
		if (command_type == "CheckCommand" ) {
			command = "$check_command$";
		} else if (command_type == "EventCommand") {
			command = "$event_command$";
		} else if (command_type == "NotificationCommand") {
			command = "$notification_command$";
		}
	} else {
		command = HttpUtility::GetLastParameter(params, "command");
	}

	/* Resolve command macro */
	String resolved_command = MacroProcessor::ResolveMacros(
		command, resolvers, checkable->GetLastCheckResult(), nullptr,
		MacroProcessor::EscapeCallback(), nullptr, false
	);

	CheckResult::Ptr cr = checkable->GetLastCheckResult();

	if (!cr)
		cr = new CheckResult();

	/* Check if resolved_command exists and it is of type command_type */
	Dictionary::Ptr execMacros = new Dictionary();

	MacroResolver::OverrideMacros = macros;
	Defer o ([]() {
		MacroResolver::OverrideMacros = nullptr;
	});

	/* Create execution parameters */
	Dictionary::Ptr execParams = new Dictionary();

	if (command_type == "CheckCommand") {
		CheckCommand::Ptr cmd = GetSingleObjectByNameUsingPermissions(CheckCommand::GetTypeName(), resolved_command, ActionsHandler::AuthenticatedApiUser);

		if (!cmd)
			return ApiActions::CreateResult(404, "Can't find a valid " + command_type + " for '" + resolved_command + "'.");
		else {
			CheckCommand::ExecuteOverride = cmd;
			Defer resetCheckCommandOverride([]() {
				CheckCommand::ExecuteOverride = nullptr;
			});
			cmd->Execute(checkable, cr, execMacros, false);
		}
	} else if (command_type == "EventCommand") {
		EventCommand::Ptr cmd = GetSingleObjectByNameUsingPermissions(EventCommand::GetTypeName(), resolved_command, ActionsHandler::AuthenticatedApiUser);

		if (!cmd)
			return ApiActions::CreateResult(404, "Can't find a valid " + command_type + " for '" + resolved_command + "'.");
		else {
			EventCommand::ExecuteOverride = cmd;
			Defer resetEventCommandOverride ([]() {
				EventCommand::ExecuteOverride = nullptr;
			});
			cmd->Execute(checkable, execMacros, false);
		}
	} else if (command_type == "NotificationCommand") {
		NotificationCommand::Ptr cmd = GetSingleObjectByNameUsingPermissions(NotificationCommand::GetTypeName(), resolved_command, ActionsHandler::AuthenticatedApiUser);

		if (!cmd)
			return ApiActions::CreateResult(404, "Can't find a valid " + command_type + " for '" + resolved_command + "'.");
		else {
			/* Get user */
			String user_string = "";

			if (params->Contains("user"))
				user_string = HttpUtility::GetLastParameter(params, "user");

			/* Resolve user macro */
			String resolved_user = MacroProcessor::ResolveMacros(
				user_string, resolvers, checkable->GetLastCheckResult(), nullptr,
				MacroProcessor::EscapeCallback(), nullptr, false
			);

			User::Ptr user = GetSingleObjectByNameUsingPermissions(User::GetTypeName(), resolved_user, ActionsHandler::AuthenticatedApiUser);

			if (!user)
				return ApiActions::CreateResult(404, "Can't find a valid user for '" + resolved_user + "'.");

			execParams->Set("user", user->GetName());

			/* Get notification */
			String notification_string = "";

			if (params->Contains("notification"))
				notification_string = HttpUtility::GetLastParameter(params, "notification");

			/* Resolve notification macro */
			String resolved_notification = MacroProcessor::ResolveMacros(
				notification_string, resolvers, checkable->GetLastCheckResult(), nullptr,
				MacroProcessor::EscapeCallback(), nullptr, false
			);

			Notification::Ptr notification = GetSingleObjectByNameUsingPermissions(Notification::GetTypeName(), resolved_notification, ActionsHandler::AuthenticatedApiUser);

			if (!notification)
				return ApiActions::CreateResult(404, "Can't find a valid notification for '" + resolved_notification + "'.");

			execParams->Set("notification", notification->GetName());

			NotificationCommand::ExecuteOverride = cmd;
			Defer resetNotificationCommandOverride ([]() {
				NotificationCommand::ExecuteOverride = nullptr;
			});

			cmd->Execute(notification, user, cr, NotificationType::NotificationCustom,
				ActionsHandler::AuthenticatedApiUser->GetName(), "", execMacros, false);
		}
	}

	/* This generates a UUID */
	String uuid = Utility::NewUniqueID();

	/* Create the deadline */
	double deadline = Utility::GetTime() + ttl;

	/* Update executions */
	Dictionary::Ptr pending_execution = new Dictionary();
	pending_execution->Set("pending", true);
	pending_execution->Set("deadline", deadline);
	Dictionary::Ptr executions = checkable->GetExecutions();

	if (!executions)
		executions = new Dictionary();

	executions->Set(uuid, pending_execution);
	checkable->SetExecutions(executions);

	/* Broadcast the update */
	Dictionary::Ptr executionsToBroadcast = new Dictionary();
	executionsToBroadcast->Set(uuid, pending_execution);
	Dictionary::Ptr updateParams = new Dictionary();
	updateParams->Set("host", host->GetName());

	if (service)
		updateParams->Set("service", service->GetShortName());

	updateParams->Set("executions", executionsToBroadcast);

	Dictionary::Ptr updateMessage = new Dictionary();
	updateMessage->Set("jsonrpc", "2.0");
	updateMessage->Set("method", "event::UpdateExecutions");
	updateMessage->Set("params", updateParams);

	MessageOrigin::Ptr origin = new MessageOrigin();
	listener->RelayMessage(origin, checkable, updateMessage, true);

	/* Populate execution parameters */
	if (command_type == "CheckCommand")
		execParams->Set("command_type", "check_command");
	else if (command_type == "EventCommand")
		execParams->Set("command_type", "event_command");
	else if (command_type == "NotificationCommand")
		execParams->Set("command_type", "notification_command");

	execParams->Set("command", resolved_command);
	execParams->Set("host", host->GetName());

	if (service)
		execParams->Set("service", service->GetShortName());

	/*
	 * If the host/service object specifies the 'check_timeout' attribute,
	 * forward this to the remote endpoint to limit the command execution time.
	 */
	if (!checkable->GetCheckTimeout().IsEmpty())
		execParams->Set("check_timeout", checkable->GetCheckTimeout());

	execParams->Set("source", uuid);
	execParams->Set("deadline", deadline);
	execParams->Set("macros", execMacros);
	execParams->Set("endpoint", resolved_endpoint);

	/* Execute command */
	bool local = endpointPtr == Endpoint::GetLocalEndpoint();

	if (local) {
		ClusterEvents::ExecuteCommandAPIHandler(origin, execParams);
	} else {
		/* Check if the child endpoints have Icinga version >= 2.13 */
		Zone::Ptr localZone = Zone::GetLocalZone();
		for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
			/* Fetch immediate child zone members */
			if (zone->GetParent() == localZone && zone->CanAccessObject(endpointPtr->GetZone())) {
				std::set<Endpoint::Ptr> endpoints = zone->GetEndpoints();

				for (const Endpoint::Ptr& childEndpoint : endpoints) {
					if (!(childEndpoint->GetCapabilities() & (uint_fast64_t)ApiCapabilities::ExecuteArbitraryCommand)) {
						/* Update execution */
						double now = Utility::GetTime();
						pending_execution->Set("exit", 126);
						pending_execution->Set("output", "Endpoint '" + childEndpoint->GetName() + "' doesn't support executing arbitrary commands.");
						pending_execution->Set("start", now);
						pending_execution->Set("end", now);
						pending_execution->Remove("pending");

						listener->RelayMessage(origin, checkable, updateMessage, true);

						Dictionary::Ptr result = new Dictionary();
						result->Set("checkable", checkable->GetName());
						result->Set("execution", uuid);
						return ApiActions::CreateResult(202, "Accepted", result);
					}
				}
			}
		}

		Dictionary::Ptr execMessage = new Dictionary();
		execMessage->Set("jsonrpc", "2.0");
		execMessage->Set("method", "event::ExecuteCommand");
		execMessage->Set("params", execParams);

		listener->RelayMessage(origin, endpointPtr->GetZone(), execMessage, true);
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("checkable", checkable->GetName());
	result->Set("execution", uuid);
	return ApiActions::CreateResult(202, "Accepted", result);
}
