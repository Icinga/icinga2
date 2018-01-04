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

#include "icinga/apievents.hpp"
#include "icinga/service.hpp"
#include "remote/eventqueue.hpp"
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
}

void ApiEvents::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("CheckResult");

	if (queues.empty())
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

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type, const MessageOrigin::Ptr& origin)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("StateChange");

	if (queues.empty())
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

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::NotificationSentToAllUsersHandler(const Notification::Ptr& notification,
	const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text, const MessageOrigin::Ptr& origin)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("Notification");

	if (queues.empty())
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

	Array::Ptr userNames = new Array();

	for (const User::Ptr& user : users) {
		userNames->Add(user->GetName());
	}

	result->Set("users", userNames);
	result->Set("notification_type", Notification::NotificationTypeToString(type));
	result->Set("author", author);
	result->Set("text", text);
	result->Set("check_result", Serialize(cr));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::FlappingChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("Flapping");

	if (queues.empty())
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
}

void ApiEvents::AcknowledgementSetHandler(const Checkable::Ptr& checkable,
	const String& author, const String& comment, AcknowledgementType type,
	bool notify, bool persistent, double expiry, const MessageOrigin::Ptr& origin)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("AcknowledgementSet");

	if (queues.empty())
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
}

void ApiEvents::AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("AcknowledgementCleared");

	if (queues.empty())
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

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}

	result->Set("acknowledgement_type", AcknowledgementNone);
}

void ApiEvents::CommentAddedHandler(const Comment::Ptr& comment)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("CommentAdded");

	if (queues.empty())
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'CommentAdded'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "CommentAdded");
	result->Set("timestamp", Utility::GetTime());

	result->Set("comment", Serialize(comment, FAConfig | FAState));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::CommentRemovedHandler(const Comment::Ptr& comment)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("CommentRemoved");

	if (queues.empty())
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'CommentRemoved'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "CommentRemoved");
	result->Set("timestamp", Utility::GetTime());

	result->Set("comment", Serialize(comment, FAConfig | FAState));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::DowntimeAddedHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeAdded");

	if (queues.empty())
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeAdded'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "DowntimeAdded");
	result->Set("timestamp", Utility::GetTime());

	result->Set("downtime", Serialize(downtime, FAConfig | FAState));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::DowntimeRemovedHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeRemoved");

	if (queues.empty())
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeRemoved'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "DowntimeRemoved");
	result->Set("timestamp", Utility::GetTime());

	result->Set("downtime", Serialize(downtime, FAConfig | FAState));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::DowntimeStartedHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeStarted");

	if (queues.empty())
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeStarted'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "DowntimeStarted");
	result->Set("timestamp", Utility::GetTime());

	result->Set("downtime", Serialize(downtime, FAConfig | FAState));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}

void ApiEvents::DowntimeTriggeredHandler(const Downtime::Ptr& downtime)
{
	std::vector<EventQueue::Ptr> queues = EventQueue::GetQueuesForType("DowntimeTriggered");

	if (queues.empty())
		return;

	Log(LogDebug, "ApiEvents", "Processing event type 'DowntimeTriggered'.");

	Dictionary::Ptr result = new Dictionary();
	result->Set("type", "DowntimeTriggered");
	result->Set("timestamp", Utility::GetTime());

	result->Set("downtime", Serialize(downtime, FAConfig | FAState));

	for (const EventQueue::Ptr& queue : queues) {
		queue->ProcessEvent(result);
	}
}
