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

#include "icinga/apievents.hpp"
#include "icinga/service.hpp"
#include "icinga/perfdatavalue.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "remote/zone.hpp"
#include "remote/apifunction.hpp"
#include "base/application.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/initialize.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include <fstream>

using namespace icinga;

INITIALIZE_ONCE(&ApiEvents::StaticInitialize);

REGISTER_APIFUNCTION(CheckResult, event, &ApiEvents::CheckResultAPIHandler);
REGISTER_APIFUNCTION(SetNextCheck, event, &ApiEvents::NextCheckChangedAPIHandler);
REGISTER_APIFUNCTION(SetNextNotification, event, &ApiEvents::NextNotificationChangedAPIHandler);
REGISTER_APIFUNCTION(SetForceNextCheck, event, &ApiEvents::ForceNextCheckChangedAPIHandler);
REGISTER_APIFUNCTION(SetForceNextNotification, event, &ApiEvents::ForceNextNotificationChangedAPIHandler);
REGISTER_APIFUNCTION(AddComment, event, &ApiEvents::CommentAddedAPIHandler);
REGISTER_APIFUNCTION(RemoveComment, event, &ApiEvents::CommentRemovedAPIHandler);
REGISTER_APIFUNCTION(AddDowntime, event, &ApiEvents::DowntimeAddedAPIHandler);
REGISTER_APIFUNCTION(RemoveDowntime, event, &ApiEvents::DowntimeRemovedAPIHandler);
REGISTER_APIFUNCTION(SetAcknowledgement, event, &ApiEvents::AcknowledgementSetAPIHandler);
REGISTER_APIFUNCTION(ClearAcknowledgement, event, &ApiEvents::AcknowledgementClearedAPIHandler);
REGISTER_APIFUNCTION(UpdateRepository, event, &ApiEvents::UpdateRepositoryAPIHandler);
REGISTER_APIFUNCTION(ExecuteCommand, event, &ApiEvents::ExecuteCommandAPIHandler);

static Timer::Ptr l_RepositoryTimer;

void ApiEvents::StaticInitialize(void)
{
	Checkable::OnNewCheckResult.connect(&ApiEvents::CheckResultHandler);
	Checkable::OnNextCheckChanged.connect(&ApiEvents::NextCheckChangedHandler);
	Notification::OnNextNotificationChanged.connect(&ApiEvents::NextNotificationChangedHandler);
	Checkable::OnForceNextCheckChanged.connect(&ApiEvents::ForceNextCheckChangedHandler);
	Checkable::OnForceNextNotificationChanged.connect(&ApiEvents::ForceNextNotificationChangedHandler);

	Checkable::OnCommentAdded.connect(&ApiEvents::CommentAddedHandler);
	Checkable::OnCommentRemoved.connect(&ApiEvents::CommentRemovedHandler);
	Checkable::OnDowntimeAdded.connect(&ApiEvents::DowntimeAddedHandler);
	Checkable::OnDowntimeRemoved.connect(&ApiEvents::DowntimeRemovedHandler);
	Checkable::OnAcknowledgementSet.connect(&ApiEvents::AcknowledgementSetHandler);
	Checkable::OnAcknowledgementCleared.connect(&ApiEvents::AcknowledgementClearedHandler);

	l_RepositoryTimer = new Timer();
	l_RepositoryTimer->SetInterval(30);
	l_RepositoryTimer->OnTimerExpired.connect(boost::bind(&ApiEvents::RepositoryTimerHandler));
	l_RepositoryTimer->Start();
	l_RepositoryTimer->Reschedule(0);
}

Dictionary::Ptr ApiEvents::MakeCheckResultMessage(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
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

void ApiEvents::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr message = MakeCheckResultMessage(checkable, cr);
	listener->RelayMessage(origin, checkable, message, true);
}

Value ApiEvents::CheckResultAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'check result' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	CheckResult::Ptr cr = new CheckResult();

	Dictionary::Ptr vcr = params->Get("cr");
	Array::Ptr vperf = vcr->Get("performance_data");
	vcr->Remove("performance_data");

	Deserialize(cr, params->Get("cr"), true);

	Array::Ptr rperf = new Array();

	if (vperf) {
		ObjectLock olock(vperf);
		BOOST_FOREACH(const Value& vp, vperf) {
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'check result' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	if (endpoint == checkable->GetCommandEndpoint())
		checkable->ProcessCheckResult(cr);
	else
		checkable->ProcessCheckResult(cr, origin);

	return Empty;
}

void ApiEvents::NextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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

Value ApiEvents::NextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'next check changed' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetNextCheck(params->Get("next_check"), false, origin);

	return Empty;
}

void ApiEvents::NextNotificationChangedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin)
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

Value ApiEvents::NextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'next notification changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	if (!params)
		return Empty;

	Notification::Ptr notification = Notification::GetByName(params->Get("notification"));

	if (!notification)
		return Empty;

	if (origin->FromZone && !origin->FromZone->CanAccessObject(notification)) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'next notification changed' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	notification->SetNextNotification(params->Get("next_notification"), false, origin);

	return Empty;
}

void ApiEvents::ForceNextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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

Value ApiEvents::ForceNextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'force next check' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetForceNextCheck(params->Get("forced"), false, origin);

	return Empty;
}

void ApiEvents::ForceNextNotificationChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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

Value ApiEvents::ForceNextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'force next notification' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetForceNextNotification(params->Get("forced"), false, origin);

	return Empty;
}

void ApiEvents::CommentAddedHandler(const Checkable::Ptr& checkable, const Comment::Ptr& comment, const MessageOrigin::Ptr& origin)
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
	params->Set("comment", Serialize(comment));

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::AddComment");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ApiEvents::CommentAddedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'comment added' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'comment added' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	Comment::Ptr comment = new Comment();
	Deserialize(comment, params->Get("comment"), true);

	checkable->AddComment(comment->GetEntryType(), comment->GetAuthor(),
	    comment->GetText(), comment->GetExpireTime(), comment->GetId(), origin);

	return Empty;
}

void ApiEvents::CommentRemovedHandler(const Checkable::Ptr& checkable, const Comment::Ptr& comment, const MessageOrigin::Ptr& origin)
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
	params->Set("id", comment->GetId());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::RemoveComment");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ApiEvents::CommentRemovedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'comment removed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'comment removed' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->RemoveComment(params->Get("id"), origin);

	return Empty;
}

void ApiEvents::DowntimeAddedHandler(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, const MessageOrigin::Ptr& origin)
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
	params->Set("downtime", Serialize(downtime));

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::AddDowntime");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ApiEvents::DowntimeAddedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'downtime added' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'downtime added' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	Downtime::Ptr downtime = new Downtime();
	Deserialize(downtime, params->Get("downtime"), true);

	checkable->AddDowntime(downtime->GetAuthor(), downtime->GetComment(),
	    downtime->GetStartTime(), downtime->GetEndTime(),
	    downtime->GetFixed(), downtime->GetTriggeredBy(),
	    downtime->GetDuration(), downtime->GetScheduledBy(),
	    downtime->GetId(), origin);

	return Empty;
}

void ApiEvents::DowntimeRemovedHandler(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, const MessageOrigin::Ptr& origin)
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
	params->Set("id", downtime->GetId());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::RemoveDowntime");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ApiEvents::DowntimeRemovedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'downtime removed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'downtime removed' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->RemoveDowntime(params->Get("id"), false, origin);

	return Empty;
}

void ApiEvents::AcknowledgementSetHandler(const Checkable::Ptr& checkable,
    const String& author, const String& comment, AcknowledgementType type,
    bool notify, double expiry, const MessageOrigin::Ptr& origin)
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

Value ApiEvents::AcknowledgementSetAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'acknowledgement set' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->AcknowledgeProblem(params->Get("author"), params->Get("comment"),
	    static_cast<AcknowledgementType>(static_cast<int>(params->Get("acktype"))),
	    params->Get("notify"), params->Get("expiry"), origin);

	return Empty;
}

void ApiEvents::AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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

Value ApiEvents::AcknowledgementClearedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiEvents")
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
		Log(LogNotice, "ApiEvents")
		    << "Discarding 'acknowledgement cleared' message from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->ClearAcknowledgement(origin);

	return Empty;
}

Value ApiEvents::ExecuteCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr sourceEndpoint = origin->FromClient->GetEndpoint();

	if (!sourceEndpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		Log(LogNotice, "ApiEvents")
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
			Log(LogWarning, "ApiEvents")
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

void ApiEvents::RepositoryTimerHandler(void)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr repository = new Dictionary();

	BOOST_FOREACH(const Host::Ptr& host, ConfigType::GetObjectsByType<Host>()) {
		Array::Ptr services = new Array();

		BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
			services->Add(service->GetShortName());
		}

		repository->Set(host->GetName(), services);
	}

	Endpoint::Ptr my_endpoint = Endpoint::GetLocalEndpoint();

	if (!my_endpoint) {
		Log(LogWarning, "ApiEvents", "No local endpoint defined. Bailing out.");
		return;
	}

	Zone::Ptr my_zone = my_endpoint->GetZone();

	if (!my_zone)
		return;

	Dictionary::Ptr params = new Dictionary();
	params->Set("seen", Utility::GetTime());
	params->Set("endpoint", my_endpoint->GetName());

	Zone::Ptr parent_zone = my_zone->GetParent();
	if (parent_zone)
		params->Set("parent_zone", parent_zone->GetName());

	params->Set("zone", my_zone->GetName());
	params->Set("repository", repository);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::UpdateRepository");
	message->Set("params", params);

	listener->RelayMessage(MessageOrigin::Ptr(), my_zone, message, false);
}

String ApiEvents::GetRepositoryDir(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/api/repository/";
}

Value ApiEvents::UpdateRepositoryAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	if (!params)
		return Empty;

	Value vrepository = params->Get("repository");
	if (vrepository.IsEmpty() || !vrepository.IsObjectType<Dictionary>())
		return Empty;

	String repositoryFile = GetRepositoryDir() + SHA256(params->Get("endpoint")) + ".repo";
	String repositoryTempFile = repositoryFile + ".tmp";

	std::ofstream fp(repositoryTempFile.CStr(), std::ofstream::out | std::ostream::trunc);
	fp << JsonEncode(params);
	fp.close();

#ifdef _WIN32
	_unlink(repositoryFile.CStr());
#endif /* _WIN32 */

	if (rename(repositoryTempFile.CStr(), repositoryFile.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(repositoryTempFile));
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::UpdateRepository");
	message->Set("params", params);

	listener->RelayMessage(origin, Zone::GetLocalZone(), message, true);

	return Empty;
}

