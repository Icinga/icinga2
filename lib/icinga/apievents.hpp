// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef APIEVENTS_H
#define APIEVENTS_H

#include "remote/eventqueue.hpp"
#include "icinga/checkable.hpp"
#include "icinga/host.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class ApiEvents
{
public:
	static void StaticInitialize();

	static void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin);
	static void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type, const MessageOrigin::Ptr& origin);


	static void NotificationSentToAllUsersHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
		const std::set<User::Ptr>& users, NotificationType type, const CheckResult::Ptr& cr, const String& author,
		const String& text, const MessageOrigin::Ptr& origin);

	static void FlappingChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);

	static void AcknowledgementSetHandler(const Checkable::Ptr& checkable,
		const String& author, const String& comment, AcknowledgementType type,
		bool notify, bool persistent, double, double expiry, const MessageOrigin::Ptr& origin);
	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const String& removedBy, double, const MessageOrigin::Ptr& origin);

	static void CommentAddedHandler(const Comment::Ptr& comment);
	static void CommentRemovedHandler(const Comment::Ptr& comment);

	static void DowntimeAddedHandler(const Downtime::Ptr& downtime);
	static void DowntimeRemovedHandler(const Downtime::Ptr& downtime);
	static void DowntimeStartedHandler(const Downtime::Ptr& downtime);
	static void DowntimeTriggeredHandler(const Downtime::Ptr& downtime);

	static void OnActiveChangedHandler(const ConfigObject::Ptr& object, const Value&);
	static void OnVersionChangedHandler(const ConfigObject::Ptr& object, const Value&);
	static void SendObjectChangeEvent(const ConfigObject::Ptr& object, const EventType& eventType, const String& eventQueue);
};

}

#endif /* APIEVENTS_H */
