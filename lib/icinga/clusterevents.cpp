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

#include "icinga/clusterevents.hpp"
#include "icinga/service.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "remote/zone.hpp"
#include "remote/apifunction.hpp"
#include "remote/eventqueue.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/exception.hpp"
#include "base/initialize.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include <fstream>

using namespace icinga;

INITIALIZE_ONCE(&ClusterEvents::StaticInitialize);

REGISTER_APIFUNCTION(CheckResult, event, &ClusterEvents::CheckResultAPIHandler);
REGISTER_APIFUNCTION(SetNextCheck, event, &ClusterEvents::NextCheckChangedAPIHandler);
REGISTER_APIFUNCTION(SetNextNotification, event, &ClusterEvents::NextNotificationChangedAPIHandler);
REGISTER_APIFUNCTION(SetForceNextCheck, event, &ClusterEvents::ForceNextCheckChangedAPIHandler);
REGISTER_APIFUNCTION(SetForceNextNotification, event, &ClusterEvents::ForceNextNotificationChangedAPIHandler);
REGISTER_APIFUNCTION(SetAcknowledgement, event, &ClusterEvents::AcknowledgementSetAPIHandler);
REGISTER_APIFUNCTION(ClearAcknowledgement, event, &ClusterEvents::AcknowledgementClearedAPIHandler);
REGISTER_APIFUNCTION(ExecuteCommand, event, &ClusterEvents::ExecuteCommandAPIHandler);
REGISTER_APIFUNCTION(SendNotifications, event, &ClusterEvents::SendNotificationsAPIHandler);
REGISTER_APIFUNCTION(NotificationSentUser, event, &ClusterEvents::NotificationSentUserAPIHandler);
REGISTER_APIFUNCTION(NotificationSentToAllUsers, event, &ClusterEvents::NotificationSentToAllUsersAPIHandler);

void ClusterEvents::StaticInitialize()
{
	Checkable::OnNewCheckResult.connect(&ClusterEvents::CheckResultHandler);
	Checkable::OnNextCheckChanged.connect(&ClusterEvents::NextCheckChangedHandler);
	Notification::OnNextNotificationChanged.connect(&ClusterEvents::NextNotificationChangedHandler);
	Checkable::OnForceNextCheckChanged.connect(&ClusterEvents::ForceNextCheckChangedHandler);
	Checkable::OnForceNextNotificationChanged.connect(&ClusterEvents::ForceNextNotificationChangedHandler);
	Checkable::OnNotificationsRequested.connect(&ClusterEvents::SendNotificationsHandler);
	Checkable::OnNotificationSentToUser.connect(&ClusterEvents::NotificationSentUserHandler);
	Checkable::OnNotificationSentToAllUsers.connect(&ClusterEvents::NotificationSentToAllUsersHandler);

	Checkable::OnAcknowledgementSet.connect(&ClusterEvents::AcknowledgementSetHandler);
	Checkable::OnAcknowledgementCleared.connect(&ClusterEvents::AcknowledgementClearedHandler);
}

Dictionary::Ptr ClusterEvents::MakeCheckResultMessage(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::CheckResult");

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	else {
		Value agent_service_name = checkable->GetExtension("agent_service_name");

		if (!agent_service_name.IsEmpty())
			params->Set("service", agent_service_name);
	}
	params->Set("cr", Serialize(cr));

	message->Set("params", params);

	return message;
}

void ClusterEvents::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr message = MakeCheckResultMessage(checkable, cr);
	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::CheckResultAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'check result' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	CheckResult::Ptr cr;
	Array::Ptr vperf;

	if (params->Contains("cr")) {
		cr = new CheckResult();
		Dictionary::Ptr vcr = params->Get("cr");

		if (vcr && vcr->Contains("performance_data")) {
			vperf = vcr->Get("performance_data");

			if (vperf)
				vcr->Remove("performance_data");

			Deserialize(cr, vcr, true);
		}
	}

	if (!cr)
		return Empty;

	Array::Ptr rperf = new Array();

	if (vperf) {
		ObjectLock olock(vperf);
		for (const Value& vp : vperf) {
			Value p;

			if (vp.IsObjectType<Dictionary>()) {
				PerfdataValue::Ptr val = new PerfdataValue();
				Deserialize(val, vp, true);
				rperf->Add(val);
			} else
				rperf->Add(vp);
		}
	}

	cr->SetPerformanceData(rperf);

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable) && endpoint != checkable->GetCommandEndpoint()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'check result' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	if (!checkable->IsPaused() && Zone::GetLocalZone() == checkable->GetZone() && endpoint == checkable->GetCommandEndpoint())
		checkable->ProcessCheckResult(cr);
	else
		checkable->ProcessCheckResult(cr, origin);

	return Empty;
}

void ClusterEvents::NextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	params->Set("next_check", checkable->GetNextCheck());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetNextCheck");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::NextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'next check changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'next check changed' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	double nextCheck = params->Get("next_check");

	if (nextCheck < Application::GetStartTime() + 60)
		return Empty;

	checkable->SetNextCheck(params->Get("next_check"), false, origin);

	return Empty;
}

void ClusterEvents::NextNotificationChangedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr params = new Dictionary();
	params->Set("notification", notification->GetName());
	params->Set("next_notification", notification->GetNextNotification());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetNextNotification");
	message->Set("params", params);

	listener->RelayMessage(origin, notification, message, true);
}

Value ClusterEvents::NextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'next notification changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Notification::Ptr notification = Notification::GetByName(params->Get("notification"));

	if (!notification)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(notification)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'next notification changed' message for notification '" << notification->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	double nextNotification = params->Get("next_notification");

	if (nextNotification < Utility::GetTime())
		return Empty;

	notification->SetNextNotification(nextNotification, false, origin);

	return Empty;
}

void ClusterEvents::ForceNextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	params->Set("forced", checkable->GetForceNextCheck());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetForceNextCheck");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::ForceNextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'force next check changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'force next check' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetForceNextCheck(params->Get("forced"), false, origin);

	return Empty;
}

void ClusterEvents::ForceNextNotificationChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	params->Set("forced", checkable->GetForceNextNotification());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetForceNextNotification");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::ForceNextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'force next notification changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'force next notification' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetForceNextNotification(params->Get("forced"), false, origin);

	return Empty;
}

void ClusterEvents::AcknowledgementSetHandler(const Checkable::Ptr& checkable,
	const String& author, const String& comment, AcknowledgementType type,
	bool notify, bool persistent, double expiry, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	params->Set("author", author);
	params->Set("comment", comment);
	params->Set("acktype", type);
	params->Set("notify", notify);
	params->Set("expiry", expiry);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetAcknowledgement");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::AcknowledgementSetAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'acknowledgement set' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'acknowledgement set' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->AcknowledgeProblem(params->Get("author"), params->Get("comment"),
		static_cast<AcknowledgementType>(static_cast<int>(params->Get("acktype"))),
		params->Get("notify"), params->Get("persistent"), params->Get("expiry"), origin);

	return Empty;
}

void ClusterEvents::AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::ClearAcknowledgement");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::AcknowledgementClearedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'acknowledgement cleared' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'acknowledgement cleared' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->ClearAcknowledgement(origin);

	return Empty;
}

Value ClusterEvents::ExecuteCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr sourceEndpoint = origin->FromClient->GetEndpoint();

	if (!sourceEndpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'execute command' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return Empty;
	}

	if (!listener->GetAcceptCommands()) {
		Log(LogWarning, "ApiListener")
			<< "Ignoring command. '" << listener->GetName() << "' does not accept commands.";

		Host::Ptr host = new Host();
		Dictionary::Ptr attrs = new Dictionary();

		attrs->Set("__name", params->Get("host"));
		attrs->Set("type", "Host");
		attrs->Set("enable_active_checks", false);

		Deserialize(host, attrs, false, FAConfig);

		if (params->Contains("service"))
			host->SetExtension("agent_service_name", params->Get("service"));

		CheckResult::Ptr cr = new CheckResult();
		cr->SetState(ServiceUnknown);
		cr->SetOutput("Endpoint '" + Endpoint::GetLocalEndpoint()->GetName() + "' does not accept commands.");
		Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
		listener->SyncSendMessage(sourceEndpoint, message);

		return Empty;
	}

	/* use a virtual host object for executing the command */
	Host::Ptr host = new Host();
	Dictionary::Ptr attrs = new Dictionary();

	attrs->Set("__name", params->Get("host"));
	attrs->Set("type", "Host");

	Deserialize(host, attrs, false, FAConfig);

	if (params->Contains("service"))
		host->SetExtension("agent_service_name", params->Get("service"));

	String command = params->Get("command");
	String command_type = params->Get("command_type");

	if (command_type == "check_command") {
		if (!CheckCommand::GetByName(command)) {
			CheckResult::Ptr cr = new CheckResult();
			cr->SetState(ServiceUnknown);
			cr->SetOutput("Check command '" + command + "' does not exist.");
			Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
			listener->SyncSendMessage(sourceEndpoint, message);
			return Empty;
		}
	} else if (command_type == "event_command") {
		if (!EventCommand::GetByName(command)) {
			Log(LogWarning, "ClusterEvents")
				<< "Event command '" << command << "' does not exist.";
			return Empty;
		}
	} else
		return Empty;

	attrs->Set(command_type, params->Get("command"));
	attrs->Set("command_endpoint", sourceEndpoint->GetName());

	Deserialize(host, attrs, false, FAConfig);

	host->SetExtension("agent_check", true);

	Dictionary::Ptr macros = params->Get("macros");

	if (command_type == "check_command") {
		try {
			host->ExecuteRemoteCheck(macros);
		} catch (const std::exception& ex) {
			CheckResult::Ptr cr = new CheckResult();
			cr->SetState(ServiceUnknown);

			String output = "Exception occured while checking '" + host->GetName() + "': " + DiagnosticInformation(ex);
			cr->SetOutput(output);

			double now = Utility::GetTime();
			cr->SetScheduleStart(now);
			cr->SetScheduleEnd(now);
			cr->SetExecutionStart(now);
			cr->SetExecutionEnd(now);

			Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
			listener->SyncSendMessage(sourceEndpoint, message);

			Log(LogCritical, "checker", output);
		}
	} else if (command_type == "event_command") {
		host->ExecuteEventHandler(macros, true);
	}

	return Empty;
}

void ClusterEvents::SendNotificationsHandler(const Checkable::Ptr& checkable, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr message = MakeCheckResultMessage(checkable, cr);
	message->Set("method", "event::SendNotifications");

	Dictionary::Ptr params = message->Get("params");
	params->Set("type", type);
	params->Set("author", author);
	params->Set("text", text);

	listener->RelayMessage(origin, nullptr, message, true);
}

Value ClusterEvents::SendNotificationsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'send notification' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && origin->FromZone != Zone::GetLocalZone()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'send custom notification' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	CheckResult::Ptr cr;
	Array::Ptr vperf;

	if (params->Contains("cr")) {
		cr = new CheckResult();
		Dictionary::Ptr vcr = params->Get("cr");

		if (vcr && vcr->Contains("performance_data")) {
			vperf = vcr->Get("performance_data");

			if (vperf)
				vcr->Remove("performance_data");

			Deserialize(cr, vcr, true);
		}
	}

	NotificationType type = static_cast<NotificationType>(static_cast<int>(params->Get("type")));
	String author = params->Get("author");
	String text = params->Get("text");

	Checkable::OnNotificationsRequested(checkable, type, cr, author, text, origin);

	return Empty;
}

void ClusterEvents::NotificationSentUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable, const User::Ptr& user,
	NotificationType notificationType, const CheckResult::Ptr& cr, const String& author, const String& commentText, const String& command,
	const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	params->Set("notification", notification->GetName());
	params->Set("user", user->GetName());
	params->Set("type", notificationType);
	params->Set("cr", Serialize(cr));
	params->Set("author", author);
	params->Set("text", commentText);
	params->Set("command", command);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::NotificationSentUser");
	message->Set("params", params);

	listener->RelayMessage(origin, nullptr, message, true);
}

Value ClusterEvents::NotificationSentUserAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'sent notification to user' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && origin->FromZone != Zone::GetLocalZone()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'send notification to user' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	CheckResult::Ptr cr;
	Array::Ptr vperf;

	if (params->Contains("cr")) {
		cr = new CheckResult();
		Dictionary::Ptr vcr = params->Get("cr");

		if (vcr && vcr->Contains("performance_data")) {
			vperf = vcr->Get("performance_data");

			if (vperf)
				vcr->Remove("performance_data");

			Deserialize(cr, vcr, true);
		}
	}

	NotificationType type = static_cast<NotificationType>(static_cast<int>(params->Get("type")));
	String author = params->Get("author");
	String text = params->Get("text");

	Notification::Ptr notification = Notification::GetByName(params->Get("notification"));

	if (!notification)
		return Empty;

	User::Ptr user = User::GetByName(params->Get("user"));

	if (!user)
		return Empty;

	String command = params->Get("command");

	Checkable::OnNotificationSentToUser(notification, checkable, user, type, cr, author, text, command, origin);

	return Empty;
}

void ClusterEvents::NotificationSentToAllUsersHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
	NotificationType notificationType, const CheckResult::Ptr& cr, const String& author, const String& commentText, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr params = new Dictionary();
	params->Set("host", host->GetName());
	if (service)
		params->Set("service", service->GetShortName());
	params->Set("notification", notification->GetName());

	Array::Ptr ausers = new Array();
	for (const User::Ptr& user : users) {
		ausers->Add(user->GetName());
	}
	params->Set("users", ausers);

	params->Set("type", notificationType);
	params->Set("cr", Serialize(cr));
	params->Set("author", author);
	params->Set("text", commentText);

	params->Set("last_notification", notification->GetLastNotification());
	params->Set("next_notification", notification->GetNextNotification());
	params->Set("notification_number", notification->GetNotificationNumber());
	params->Set("last_problem_notification", notification->GetLastProblemNotification());
	params->Set("no_more_notifications", notification->GetNoMoreNotifications());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::NotificationSentToAllUsers");
	message->Set("params", params);

	listener->RelayMessage(origin, nullptr, message, true);
}

Value ClusterEvents::NotificationSentToAllUsersAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'sent notification to all users' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Host::Ptr host = Host::GetByName(params->Get("host"));

	if (!host)
		return Empty;

	Checkable::Ptr checkable;

	if (params->Contains("service"))
		checkable = host->GetServiceByShortName(params->Get("service"));
	else
		checkable = host;

	if (!checkable)
		return Empty;

	if (origin->FromZone && origin->FromZone != Zone::GetLocalZone()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'sent notification to all users' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	CheckResult::Ptr cr;
	Array::Ptr vperf;

	if (params->Contains("cr")) {
		cr = new CheckResult();
		Dictionary::Ptr vcr = params->Get("cr");

		if (vcr && vcr->Contains("performance_data")) {
			vperf = vcr->Get("performance_data");

			if (vperf)
				vcr->Remove("performance_data");

			Deserialize(cr, vcr, true);
		}
	}

	NotificationType type = static_cast<NotificationType>(static_cast<int>(params->Get("type")));
	String author = params->Get("author");
	String text = params->Get("text");

	Notification::Ptr notification = Notification::GetByName(params->Get("notification"));

	if (!notification)
		return Empty;

	Array::Ptr ausers = params->Get("users");

	if (!ausers)
		return Empty;

	std::set<User::Ptr> users;

	{
		ObjectLock olock(ausers);
		for (const String& auser : ausers) {
			User::Ptr user = User::GetByName(auser);

			if (!user)
				continue;

			users.insert(user);
		}
	}

	notification->SetLastNotification(params->Get("last_notification"));
	notification->SetNextNotification(params->Get("next_notification"));
	notification->SetNotificationNumber(params->Get("notification_number"));
	notification->SetLastProblemNotification(params->Get("last_problem_notification"));
	notification->SetNoMoreNotifications(params->Get("no_more_notifications"));

	Array::Ptr notifiedProblemUsers = new Array();
	for (const User::Ptr& user : users) {
		notifiedProblemUsers->Add(user->GetName());
	}

	notification->SetNotifiedProblemUsers(notifiedProblemUsers);

	Checkable::OnNotificationSentToAllUsers(notification, checkable, users, type, cr, author, text, origin);

	return Empty;
}
