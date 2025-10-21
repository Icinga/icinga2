/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
REGISTER_APIFUNCTION(SetLastCheckStarted, event, &ClusterEvents::LastCheckStartedChangedAPIHandler);
REGISTER_APIFUNCTION(SetStateBeforeSuppression, event, &ClusterEvents::StateBeforeSuppressionChangedAPIHandler);
REGISTER_APIFUNCTION(SetSuppressedNotifications, event, &ClusterEvents::SuppressedNotificationsChangedAPIHandler);
REGISTER_APIFUNCTION(SetSuppressedNotificationTypes, event, &ClusterEvents::SuppressedNotificationTypesChangedAPIHandler);
REGISTER_APIFUNCTION(SetNextNotification, event, &ClusterEvents::NextNotificationChangedAPIHandler);
REGISTER_APIFUNCTION(UpdateLastNotifiedStatePerUser, event, &ClusterEvents::LastNotifiedStatePerUserUpdatedAPIHandler);
REGISTER_APIFUNCTION(ClearLastNotifiedStatePerUser, event, &ClusterEvents::LastNotifiedStatePerUserClearedAPIHandler);
REGISTER_APIFUNCTION(SetForceNextCheck, event, &ClusterEvents::ForceNextCheckChangedAPIHandler);
REGISTER_APIFUNCTION(SetForceNextNotification, event, &ClusterEvents::ForceNextNotificationChangedAPIHandler);
REGISTER_APIFUNCTION(SetAcknowledgement, event, &ClusterEvents::AcknowledgementSetAPIHandler);
REGISTER_APIFUNCTION(ClearAcknowledgement, event, &ClusterEvents::AcknowledgementClearedAPIHandler);
REGISTER_APIFUNCTION(ExecuteCommand, event, &ClusterEvents::ExecuteCommandAPIHandler);
REGISTER_APIFUNCTION(SendNotifications, event, &ClusterEvents::SendNotificationsAPIHandler);
REGISTER_APIFUNCTION(NotificationSentUser, event, &ClusterEvents::NotificationSentUserAPIHandler);
REGISTER_APIFUNCTION(NotificationSentToAllUsers, event, &ClusterEvents::NotificationSentToAllUsersAPIHandler);
REGISTER_APIFUNCTION(ExecutedCommand, event, &ClusterEvents::ExecutedCommandAPIHandler);
REGISTER_APIFUNCTION(UpdateExecutions, event, &ClusterEvents::UpdateExecutionsAPIHandler);
REGISTER_APIFUNCTION(SetRemovalInfo, event, &ClusterEvents::SetRemovalInfoAPIHandler);

void ClusterEvents::StaticInitialize()
{
	Checkable::OnNewCheckResult.connect(&ClusterEvents::CheckResultHandler);
	Checkable::OnNextCheckChanged.connect([](const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin) {
		NextCheckChangedHandler(checkable, origin);
	});
	Checkable::OnLastCheckStartedChanged.connect(&ClusterEvents::LastCheckStartedChangedHandler);
	Checkable::OnStateBeforeSuppressionChanged.connect(&ClusterEvents::StateBeforeSuppressionChangedHandler);
	Checkable::OnSuppressedNotificationsChanged.connect(&ClusterEvents::SuppressedNotificationsChangedHandler);
	Notification::OnSuppressedNotificationsChanged.connect(&ClusterEvents::SuppressedNotificationTypesChangedHandler);
	Notification::OnNextNotificationChanged.connect(&ClusterEvents::NextNotificationChangedHandler);
	Notification::OnLastNotifiedStatePerUserUpdated.connect(&ClusterEvents::LastNotifiedStatePerUserUpdatedHandler);
	Notification::OnLastNotifiedStatePerUserCleared.connect(&ClusterEvents::LastNotifiedStatePerUserClearedHandler);
	Checkable::OnForceNextCheckChanged.connect(&ClusterEvents::ForceNextCheckChangedHandler);
	Checkable::OnForceNextNotificationChanged.connect(&ClusterEvents::ForceNextNotificationChangedHandler);
	Checkable::OnNotificationsRequested.connect(&ClusterEvents::SendNotificationsHandler);
	Checkable::OnNotificationSentToUser.connect(&ClusterEvents::NotificationSentUserHandler);
	Checkable::OnNotificationSentToAllUsers.connect(&ClusterEvents::NotificationSentToAllUsersHandler);

	Checkable::OnAcknowledgementSet.connect(&ClusterEvents::AcknowledgementSetHandler);
	Checkable::OnAcknowledgementCleared.connect(&ClusterEvents::AcknowledgementClearedHandler);

	Comment::OnRemovalInfoChanged.connect(&ClusterEvents::SetRemovalInfoHandler);
	Downtime::OnRemovalInfoChanged.connect(&ClusterEvents::SetRemovalInfoHandler);
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

	ArrayData rperf;

	if (vperf) {
		ObjectLock olock(vperf);
		for (const Value& vp : vperf) {
			Value p;

			if (vp.IsObjectType<Dictionary>()) {
				PerfdataValue::Ptr val = new PerfdataValue();
				Deserialize(val, vp, true);
				rperf.push_back(val);
			} else
				rperf.push_back(vp);
		}
	}

	cr->SetPerformanceData(new Array(std::move(rperf)));

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
		checkable->ProcessCheckResult(cr, ApiListener::GetInstance()->GetWaitGroup());
	else
		checkable->ProcessCheckResult(cr, ApiListener::GetInstance()->GetWaitGroup(), origin);

	return Empty;
}

void ClusterEvents::NextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin, bool isFromOldClient)
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
	params->Set("origin_client_old", isFromOldClient);

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

	bool isOriginClientOld = params->Get("origin_client_old").ToBool();
	if (isOriginClientOld || endpoint->GetIcingaVersion() < 21600) { // TODO: Exact required version??
		// All older Icinga 2 versions trigger the `event::SetNextCheck` event endlessly, so we can't let the
		// `Checkable::OnNextCheckChanged` know about this change, as it would cause too much noise in Icinga DB
		// and the like. So, we just update the timestamp silently and manually call `Checkable::OnRescheduleCheck`
		// to inform the checker about this change.
		checkable->SetNextCheck(params->Get("next_check"), true, origin);
		Checkable::OnRescheduleCheck(checkable, nextCheck);
		// In case this message has to be relayed to other cluster nodes we directly call the `NextCheckChangedHandler`.
		// The `isOriginClientOld` flag is necessary e.g. in a 3-level cluster setup where a very old agent sends
		// the message to its satellite that then relays it to the master. The master then needs to know that this
		// event originates from an old node and thus has to be treated specially.
		NextCheckChangedHandler(checkable, origin, true);
	} else {
		// Peer is running the expected Icinga 2 version, so we can just do the normal flow.
		// The `Checkable::OnNextCheckChanged` signal will be emitted as usual and all listeners
		// will be informed about this change.
		checkable->SetNextCheck(params->Get("next_check"), false, origin);
	}

	return Empty;
}

void ClusterEvents::LastCheckStartedChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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
	params->Set("last_check_started", checkable->GetLastCheckStarted());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetLastCheckStarted");
	message->Set("params", params);

	listener->RelayMessage(origin, checkable, message, true);
}

Value ClusterEvents::LastCheckStartedChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'last_check_started changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

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
			<< "Discarding 'last_check_started changed' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetLastCheckStarted(params->Get("last_check_started"), false, origin);

	return Empty;
}

void ClusterEvents::StateBeforeSuppressionChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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
	params->Set("state_before_suppression", checkable->GetStateBeforeSuppression());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetStateBeforeSuppression");
	message->Set("params", params);

	listener->RelayMessage(origin, nullptr, message, true);
}

Value ClusterEvents::StateBeforeSuppressionChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'state before suppression changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

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
			<< "Discarding 'state before suppression changed' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetStateBeforeSuppression(ServiceState(int(params->Get("state_before_suppression"))), false, origin);

	return Empty;
}

void ClusterEvents::SuppressedNotificationsChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
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
	params->Set("suppressed_notifications", checkable->GetSuppressedNotifications());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetSuppressedNotifications");
	message->Set("params", params);

	listener->RelayMessage(origin, nullptr, message, true);
}

Value ClusterEvents::SuppressedNotificationsChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'suppressed notifications changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

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
			<< "Discarding 'suppressed notifications changed' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	checkable->SetSuppressedNotifications(params->Get("suppressed_notifications"), false, origin);

	return Empty;
}

void ClusterEvents::SuppressedNotificationTypesChangedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr params = new Dictionary();
	params->Set("notification", notification->GetName());
	params->Set("suppressed_notifications", notification->GetSuppressedNotifications());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetSuppressedNotificationTypes");
	message->Set("params", params);

	listener->RelayMessage(origin, nullptr, message, true);
}

Value ClusterEvents::SuppressedNotificationTypesChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'suppressed notifications changed' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	auto notification (Notification::GetByName(params->Get("notification")));

	if (!notification)
		return Empty;

	if (origin->FromZone && origin->FromZone != Zone::GetLocalZone()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'suppressed notification types changed' message for notification '" << notification->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	notification->SetSuppressedNotifications(params->Get("suppressed_notifications"), false, origin);

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

void ClusterEvents::LastNotifiedStatePerUserUpdatedHandler(const Notification::Ptr& notification, const String& user, uint_fast8_t state, const MessageOrigin::Ptr& origin)
{
	auto listener (ApiListener::GetInstance());

	if (!listener) {
		return;
	}

	Dictionary::Ptr params = new Dictionary();
	params->Set("notification", notification->GetName());
	params->Set("user", user);
	params->Set("state", state);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::UpdateLastNotifiedStatePerUser");
	message->Set("params", params);

	listener->RelayMessage(origin, notification, message, true);
}

Value ClusterEvents::LastNotifiedStatePerUserUpdatedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	auto endpoint (origin->FromClient->GetEndpoint());

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'last notified state of user updated' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";

		return Empty;
	}

	if (origin->FromZone && origin->FromZone != Zone::GetLocalZone()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'last notified state of user updated' message from '"
			<< origin->FromClient->GetIdentity() << "': Unauthorized access.";

		return Empty;
	}

	auto notification (Notification::GetByName(params->Get("notification")));

	if (!notification) {
		return Empty;
	}

	auto state (params->Get("state"));

	if (!state.IsNumber()) {
		return Empty;
	}

	notification->GetLastNotifiedStatePerUser()->Set(params->Get("user"), state);
	Notification::OnLastNotifiedStatePerUserUpdated(notification, params->Get("user"), state, origin);

	return Empty;
}

void ClusterEvents::LastNotifiedStatePerUserClearedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin)
{
	auto listener (ApiListener::GetInstance());

	if (!listener) {
		return;
	}

	Dictionary::Ptr params = new Dictionary();
	params->Set("notification", notification->GetName());

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::ClearLastNotifiedStatePerUser");
	message->Set("params", params);

	listener->RelayMessage(origin, notification, message, true);
}

Value ClusterEvents::LastNotifiedStatePerUserClearedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	auto endpoint (origin->FromClient->GetEndpoint());

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'last notified state of user cleared' message from '"
			<< origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";

		return Empty;
	}

	if (origin->FromZone && origin->FromZone != Zone::GetLocalZone()) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'last notified state of user cleared' message from '"
			<< origin->FromClient->GetIdentity() << "': Unauthorized access.";

		return Empty;
	}

	auto notification (Notification::GetByName(params->Get("notification")));

	if (!notification) {
		return Empty;
	}

	notification->GetLastNotifiedStatePerUser()->Clear();
	Notification::OnLastNotifiedStatePerUserCleared(notification, origin);

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
	bool notify, bool persistent, double changeTime, double expiry, const MessageOrigin::Ptr& origin)
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
	params->Set("persistent", persistent);
	params->Set("expiry", expiry);
	params->Set("change_time", changeTime);

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

	ObjectLock oLock (checkable);

	if (checkable->IsAcknowledged()) {
		Log(LogWarning, "ClusterEvents")
			<< "Discarding 'acknowledgement set' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Checkable is already acknowledged.";
		return Empty;
	}

	checkable->AcknowledgeProblem(params->Get("author"), params->Get("comment"),
		static_cast<AcknowledgementType>(static_cast<int>(params->Get("acktype"))),
		params->Get("notify"), params->Get("persistent"), params->Get("change_time"), params->Get("expiry"), origin);

	return Empty;
}

void ClusterEvents::AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const String& removedBy, double changeTime, const MessageOrigin::Ptr& origin)
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
	params->Set("author", removedBy);
	params->Set("change_time", changeTime);

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

	checkable->ClearAcknowledgement(params->Get("author"), params->Get("change_time"), origin);

	return Empty;
}

Value ClusterEvents::ExecuteCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

	if (!origin->IsLocal()) {
		Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

		/* Discard messages from anonymous clients */
		if (!endpoint) {
			Log(LogNotice, "ClusterEvents") << "Discarding 'execute command' message from '"
				<< origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
			return Empty;
		}

		Zone::Ptr originZone = endpoint->GetZone();

		Zone::Ptr localZone = Zone::GetLocalZone();
		bool fromLocalZone = originZone == localZone;

		Zone::Ptr parentZone = localZone->GetParent();
		bool fromParentZone = parentZone && originZone == parentZone;

		if (!fromLocalZone && !fromParentZone) {
			Log(LogNotice, "ClusterEvents") << "Discarding 'execute command' message from '"
				<< origin->FromClient->GetIdentity() << "': Unauthorized access.";
			return Empty;
		}
	}

	String executionUuid = params->Get("source");

	if (params->Contains("endpoint")) {
		Endpoint::Ptr execEndpoint = Endpoint::GetByName(params->Get("endpoint"));

		if (!execEndpoint) {
			Log(LogWarning, "ClusterEvents")
				<< "Discarding 'execute command' message " << executionUuid
				<< ": Endpoint " << params->Get("endpoint") << " does not exist";
			return Empty;
		}

		if (execEndpoint != Endpoint::GetLocalEndpoint()) {
			Zone::Ptr endpointZone = execEndpoint->GetZone();
			Zone::Ptr localZone = Zone::GetLocalZone();

			if (!endpointZone->IsChildOf(localZone)) {
				return Empty;
			}

			/* Check if the child endpoints have Icinga version >= 2.13 */
			for (const Zone::Ptr &zone : ConfigType::GetObjectsByType<Zone>()) {
				/* Fetch immediate child zone members */
				if (zone->GetParent() == localZone && zone->CanAccessObject(endpointZone)) {
					auto endpoints (zone->GetEndpoints());

					for (const Endpoint::Ptr &childEndpoint : endpoints) {
						if (!(childEndpoint->GetCapabilities() & (uint_fast64_t)ApiCapabilities::ExecuteArbitraryCommand)) {
							double now = Utility::GetTime();
							Dictionary::Ptr executedParams = new Dictionary();
							executedParams->Set("execution", executionUuid);
							executedParams->Set("host", params->Get("host"));

							if (params->Contains("service"))
								executedParams->Set("service", params->Get("service"));

							executedParams->Set("exit", 126);
							executedParams->Set("output",
								"Endpoint '" + childEndpoint->GetName() + "' doesn't support executing arbitrary commands.");
							executedParams->Set("start", now);
							executedParams->Set("end", now);

							Dictionary::Ptr executedMessage = new Dictionary();
							executedMessage->Set("jsonrpc", "2.0");
							executedMessage->Set("method", "event::ExecutedCommand");
							executedMessage->Set("params", executedParams);

							listener->RelayMessage(nullptr, nullptr, executedMessage, true);
							return Empty;
						}
					}

					Checkable::Ptr checkable;
					Host::Ptr host = Host::GetByName(params->Get("host"));
					if (!host) {
						Log(LogWarning, "ClusterEvents")
							<< "Discarding 'execute command' message " << executionUuid
							<< ": host " << params->Get("host") << " does not exist";
						return Empty;
					}

					if (params->Contains("service"))
						checkable = host->GetServiceByShortName(params->Get("service"));
					else
						checkable = host;

					if (!checkable) {
						String checkableName = host->GetName();
						if (params->Contains("service"))
							checkableName += "!" + params->Get("service");

						Log(LogWarning, "ClusterEvents")
							<< "Discarding 'execute command' message " << executionUuid
							<< ": " << checkableName << " does not exist";
						return Empty;
					}

					/* Return an error when the endpointZone is different than the child zone and
					 * the child zone can't access the checkable.
					 * The zones are checked to allow for the case where command_endpoint is specified in the checkable
					 * but checkable is not actually present in the agent.
					 */
					if (!zone->CanAccessObject(checkable) && zone != endpointZone) {
						double now = Utility::GetTime();
						Dictionary::Ptr executedParams = new Dictionary();
						executedParams->Set("execution", executionUuid);
						executedParams->Set("host", params->Get("host"));

						if (params->Contains("service"))
							executedParams->Set("service", params->Get("service"));

						executedParams->Set("exit", 126);
						executedParams->Set(
							"output",
							"Zone '" + zone->GetName() + "' cannot access to checkable '" + checkable->GetName() + "'."
						);
						executedParams->Set("start", now);
						executedParams->Set("end", now);

						Dictionary::Ptr executedMessage = new Dictionary();
						executedMessage->Set("jsonrpc", "2.0");
						executedMessage->Set("method", "event::ExecutedCommand");
						executedMessage->Set("params", executedParams);

						listener->RelayMessage(nullptr, nullptr, executedMessage, true);
						return Empty;
					}
				}
			}

			Dictionary::Ptr execMessage = new Dictionary();
			execMessage->Set("jsonrpc", "2.0");
			execMessage->Set("method", "event::ExecuteCommand");
			execMessage->Set("params", params);

			listener->RelayMessage(origin, endpointZone, execMessage, true);
			return Empty;
		}
	}

	EnqueueCheck(origin, params);

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

	ArrayData ausers;
	for (const User::Ptr& user : users) {
		ausers.push_back(user->GetName());
	}
	params->Set("users", new Array(std::move(ausers)));

	params->Set("type", notificationType);
	params->Set("cr", Serialize(cr));
	params->Set("author", author);
	params->Set("text", commentText);

	params->Set("last_notification", notification->GetLastNotification());
	params->Set("next_notification", notification->GetNextNotification());
	params->Set("notification_number", notification->GetNotificationNumber());
	params->Set("last_problem_notification", notification->GetLastProblemNotification());
	params->Set("no_more_notifications", notification->GetNoMoreNotifications());
	params->Set("notified_problem_users", notification->GetNotifiedProblemUsers());

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
		for (String auser : ausers) {
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

	if (params->Contains("notified_problem_users")) {
		notification->SetNotifiedProblemUsers(params->Get("notified_problem_users"));
	} else {
		ArrayData notifiedProblemUsers;
		for (const User::Ptr& user : users) {
			notifiedProblemUsers.push_back(user->GetName());
		}

		notification->SetNotifiedProblemUsers(new Array(std::move(notifiedProblemUsers)));
	}

	Checkable::OnNotificationSentToAllUsers(notification, checkable, users, type, cr, author, text, origin);

	return Empty;
}

Value ClusterEvents::ExecutedCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

	Endpoint::Ptr endpoint;

	if (origin->FromClient) {
		endpoint = origin->FromClient->GetEndpoint();
	} else if (origin->IsLocal()) {
		endpoint = Endpoint::GetLocalEndpoint();
	}

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message from '" << origin->FromClient->GetIdentity()
			<< "': Invalid endpoint origin (client not allowed).";

		return Empty;
	}

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

	ObjectLock oLock (checkable);

	if (!params->Contains("execution")) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Execution UUID missing.";
		return Empty;
	}

	String uuid = params->Get("execution");

	Dictionary::Ptr executions = checkable->GetExecutions();

	if (!executions) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Execution '" << uuid << "' missing.";
		return Empty;
	}

	Dictionary::Ptr execution = executions->Get(uuid);

	if (!execution) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Execution '" << uuid << "' missing.";
		return Empty;
	}

	Endpoint::Ptr command_endpoint = Endpoint::GetByName(execution->Get("endpoint"));
	if (!command_endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message from '" << origin->FromClient->GetIdentity()
			<< "': Command endpoint does not exists.";

		return Empty;
	}

	if (origin->FromZone && !command_endpoint->GetZone()->IsChildOf(origin->FromZone)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	if (params->Contains("exit"))
		execution->Set("exit", params->Get("exit"));

	if (params->Contains("output"))
		execution->Set("output", params->Get("output"));

	if (params->Contains("start"))
		execution->Set("start", params->Get("start"));

	if (params->Contains("end"))
		execution->Set("end", params->Get("end"));

	execution->Remove("pending");

	/* Broadcast the update */
	Dictionary::Ptr executionsToBroadcast = new Dictionary();
	executionsToBroadcast->Set(uuid, execution);
	Dictionary::Ptr updateParams = new Dictionary();
	updateParams->Set("host", host->GetName());

	if (params->Contains("service"))
		updateParams->Set("service", params->Get("service"));

	updateParams->Set("executions", executionsToBroadcast);

	Dictionary::Ptr updateMessage = new Dictionary();
	updateMessage->Set("jsonrpc", "2.0");
	updateMessage->Set("method", "event::UpdateExecutions");
	updateMessage->Set("params", updateParams);

	listener->RelayMessage(nullptr, checkable, updateMessage, true);

	return Empty;
}

Value ClusterEvents::UpdateExecutionsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message from '" << origin->FromClient->GetIdentity()
			<< "': Invalid endpoint origin (client not allowed).";

		return Empty;
	}

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

	ObjectLock oLock (checkable);

	if (origin->FromZone && !origin->FromZone->CanAccessObject(checkable)) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'update executions API handler' message for checkable '" << checkable->GetName()
			<< "' from '" << origin->FromClient->GetIdentity() << "': Unauthorized access.";
		return Empty;
	}

	Dictionary::Ptr executions = checkable->GetExecutions();

	if (!executions)
		executions = new Dictionary();

	Dictionary::Ptr newExecutions = params->Get("executions");
	newExecutions->CopyTo(executions);
	checkable->SetExecutions(executions);

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

	Dictionary::Ptr updateMessage = new Dictionary();
	updateMessage->Set("jsonrpc", "2.0");
	updateMessage->Set("method", "event::UpdateExecutions");
	updateMessage->Set("params", params);

	listener->RelayMessage(origin, checkable, updateMessage, true);

	return Empty;
}

void ClusterEvents::SetRemovalInfoHandler(const ConfigObject::Ptr& obj, const String& removedBy, double removeTime,
	const MessageOrigin::Ptr& origin)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	Dictionary::Ptr params = new Dictionary();
	params->Set("object_type", obj->GetReflectionType()->GetName());
	params->Set("object_name", obj->GetName());
	params->Set("removed_by", removedBy);
	params->Set("remove_time", removeTime);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "event::SetRemovalInfo");
	message->Set("params", params);

	listener->RelayMessage(origin, obj, message, true);
}

Value ClusterEvents::SetRemovalInfoAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'set removal info' message from '" << origin->FromClient->GetIdentity()
			<< "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	String objectType = params->Get("object_type");
	String objectName = params->Get("object_name");
	String removedBy = params->Get("removed_by");
	double removeTime = params->Get("remove_time");

	if (objectType == Comment::GetTypeName()) {
		Comment::Ptr comment = Comment::GetByName(objectName);

		if (comment) {
			comment->SetRemovalInfo(removedBy, removeTime, origin);
		}
	} else if (objectType == Downtime::GetTypeName()) {
		Downtime::Ptr downtime = Downtime::GetByName(objectName);

		if (downtime) {
			downtime->SetRemovalInfo(removedBy, removeTime, origin);
		}
	} else {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'set removal info' message from '" << origin->FromClient->GetIdentity()
			<< "': Unknown object type.";
	}

	return Empty;
}
