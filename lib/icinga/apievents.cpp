// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/apievents.hpp"
#include "icinga/service.hpp"
#include "icinga/notificationcommand.hpp"
#include "base/initialize.hpp"
#include "base/serializer.hpp"
#include "base/logger.hpp"

using namespace icinga;

INITIALIZE_ONCE(&ApiEvents::StaticInitialize);

void ApiEvents::StaticInitialize()
{
	Checkable::OnNewCheckResult.connect(&ApiEvents::CheckResultHandler);
	Checkable::OnStateChange.connect(&ApiEvents::StateChangeHandler);
	Checkable::OnNotificationSentToAllUsers.connect(&ApiEvents::NotificationSentToAllUsersHandler);

	Checkable::OnFlappingChanged.connect(&ApiEvents::FlappingChangedHandler);

	Checkable::OnAcknowledgementSet.connect(&ApiEvents::AcknowledgementSetHandler);
	Checkable::OnAcknowledgementCleared.connect(&ApiEvents::AcknowledgementClearedHandler);

	Comment::OnCommentAdded.connect(&ApiEvents::CommentAddedHandler);
	Comment::OnCommentRemoved.connect(&ApiEvents::CommentRemovedHandler);

	Downtime::OnDowntimeAdded.connect(&ApiEvents::DowntimeAddedHandler);
	Downtime::OnDowntimeRemoved.connect(&ApiEvents::DowntimeRemovedHandler);
	Downtime::OnDowntimeStarted.connect(&ApiEvents::DowntimeStartedHandler);
	Downtime::OnDowntimeTriggered.connect(&ApiEvents::DowntimeTriggeredHandler);

	ConfigObject::OnActiveChanged.connect(&ApiEvents::OnActiveChangedHandler);
	ConfigObject::OnVersionChanged.connect(&ApiEvents::OnVersionChangedHandler);
}

void ApiEvents::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr&)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("CheckResult");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::CheckResult));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'CheckResult'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "CheckResult");
	result->Set("timestamp", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	result->Set("host", host->GetName());
	if (service)
		result->Set("service", service->GetShortName());

	result->Set("check_result", Serialize(cr));

	result->Set("downtime_depth", checkable->GetDowntimeDepth());
	result->Set("acknowledgement", checkable->IsAcknowledged());

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType, const MessageOrigin::Ptr&)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("StateChange");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::StateChange));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'StateChange'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "StateChange");
	result->Set("timestamp", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	result->Set("host", host->GetName());
	if (service)
		result->Set("service", service->GetShortName());

	result->Set("state", service ? static_cast<int>(service->GetState()) : static_cast<int>(host->GetState()));
	result->Set("state_type", checkable->GetStateType());
	result->Set("check_result", Serialize(cr));

	result->Set("downtime_depth", checkable->GetDowntimeDepth());
	result->Set("acknowledgement", checkable->IsAcknowledged());

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::NotificationSentToAllUsersHandler(const Notification::Ptr& notification,
	const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text, const MessageOrigin::Ptr&)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("Notification");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::Notification));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'Notification'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "Notification");
	result->Set("timestamp", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	result->Set("host", host->GetName());
	if (service)
		result->Set("service", service->GetShortName());

	NotificationCommand::Ptr command = notification->GetCommand();

	if (command)
		result->Set("command", command->GetName());

	ArrayData userNames;

	for (const User::Ptr& user : users) {
		userNames.push_back(user->GetName());
	}

	result->Set("users", new Array(std::move(userNames)));
	result->Set("notification_type", Notification::NotificationTypeToStringCompat(type)); //TODO: Change this to our own types.
	result->Set("author", author);
	result->Set("text", text);
	result->Set("check_result", Serialize(cr));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::FlappingChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr&)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("Flapping");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::Flapping));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'Flapping'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "Flapping");
	result->Set("timestamp", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	result->Set("host", host->GetName());
	if (service)
		result->Set("service", service->GetShortName());

	result->Set("state", service ? static_cast<int>(service->GetState()) : static_cast<int>(host->GetState()));
	result->Set("state_type", checkable->GetStateType());
	result->Set("is_flapping", checkable->IsFlapping());
	result->Set("flapping_current", checkable->GetFlappingCurrent());
	result->Set("threshold_low", checkable->GetFlappingThresholdLow());
	result->Set("threshold_high", checkable->GetFlappingThresholdHigh());

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::AcknowledgementSetHandler(const Checkable::Ptr& checkable,
	const String& author, const String& comment, AcknowledgementType type,
	bool notify, bool persistent, double, double expiry, const MessageOrigin::Ptr&)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("AcknowledgementSet");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::AcknowledgementSet));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'AcknowledgementSet'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "AcknowledgementSet");
	result->Set("timestamp", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	result->Set("host", host->GetName());
	if (service)
		result->Set("service", service->GetShortName());

	result->Set("state", service ? static_cast<int>(service->GetState()) : static_cast<int>(host->GetState()));
	result->Set("state_type", checkable->GetStateType());

	result->Set("author", author);
	result->Set("comment", comment);
	result->Set("acknowledgement_type", type);
	result->Set("notify", notify);
	result->Set("persistent", persistent);
	result->Set("expiry", expiry);

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::AcknowledgementClearedHandler(const Checkable::Ptr& checkable, [[maybe_unused]] const String& removedBy, double, const MessageOrigin::Ptr&)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("AcknowledgementCleared");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::AcknowledgementCleared));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'AcknowledgementCleared'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "AcknowledgementCleared");
	result->Set("timestamp", Utility::GetTime());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	result->Set("host", host->GetName());
	if (service)
		result->Set("service", service->GetShortName());

	result->Set("state", service ? static_cast<int>(service->GetState()) : static_cast<int>(host->GetState()));
	result->Set("state_type", checkable->GetStateType());
	result->Set("acknowledgement_type", AcknowledgementNone);

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::CommentAddedHandler(const Comment::Ptr& comment)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("CommentAdded");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::CommentAdded));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'CommentAdded'.");

	Dictionary::Ptr result = new Dictionary({
		{ "type", "CommentAdded" },
		{ "timestamp", Utility::GetTime() },
		{ "comment", Serialize(comment, FAConfig | FAState) }
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::CommentRemovedHandler(const Comment::Ptr& comment)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("CommentRemoved");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::CommentRemoved));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'CommentRemoved'.");

	Dictionary::Ptr result = new Dictionary({
		{ "type", "CommentRemoved" },
		{ "timestamp", Utility::GetTime() },
		{ "comment", Serialize(comment, FAConfig | FAState) }
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::DowntimeAddedHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeAdded");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::DowntimeAdded));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeAdded'.");

	Dictionary::Ptr result = new Dictionary({
		{ "type", "DowntimeAdded" },
		{ "timestamp", Utility::GetTime() },
		{ "downtime", Serialize(downtime, FAConfig | FAState) }
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::DowntimeRemovedHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeRemoved");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::DowntimeRemoved));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeRemoved'.");

	Dictionary::Ptr result = new Dictionary({
		{ "type", "DowntimeRemoved" },
		{ "timestamp", Utility::GetTime() },
		{ "downtime", Serialize(downtime, FAConfig | FAState) }
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::DowntimeStartedHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeStarted");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::DowntimeStarted));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeStarted'.");

	Dictionary::Ptr result = new Dictionary({
		{ "type", "DowntimeStarted" },
		{ "timestamp", Utility::GetTime() },
		{ "downtime", Serialize(downtime, FAConfig | FAState) }
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::DowntimeTriggeredHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeTriggered");
	auto inboxes (EventsRouter::GetInstance().GetInboxes(EventType::DowntimeTriggered));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeTriggered'.");

	Dictionary::Ptr result = new Dictionary({
		{ "type", "DowntimeTriggered" },
		{ "timestamp", Utility::GetTime() },
		{ "downtime", Serialize(downtime, FAConfig | FAState) }
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}

void ApiEvents::OnActiveChangedHandler(const ConfigObject::Ptr& object, const Value&)
{
	if (object->IsActive()) {
		ApiEvents::SendObjectChangeEvent(object, EventType::ObjectCreated, "ObjectCreated");
	} else if (!object->IsActive() && !object->GetExtension("ConfigObjectDeleted").IsEmpty()) {
		ApiEvents::SendObjectChangeEvent(object, EventType::ObjectDeleted, "ObjectDeleted");
	}
}

void ApiEvents::OnVersionChangedHandler(const ConfigObject::Ptr& object, const Value&)
{
	ApiEvents::SendObjectChangeEvent(object, EventType::ObjectModified, "ObjectModified");
}

void ApiEvents::SendObjectChangeEvent(const ConfigObject::Ptr& object, const EventType& eventType, const String& eventQueue) {
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType(eventQueue);
	auto inboxes (EventsRouter::GetInstance().GetInboxes(eventType));

	if (queues.empty() && !inboxes)
		return;

	Log(LogDebug, "ApiEvents") << "Processing event type '" + eventQueue + "'.";

	Dictionary::Ptr result = new Dictionary ({
		{"type", eventQueue},
		{"timestamp", Utility::GetTime()},
		{"object_type", object->GetReflectionType()->GetName()},
		{"object_name", object->GetName()},
	});

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	inboxes.Push(std::move(result));
}
