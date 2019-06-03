/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APIEVENTS_H
#define APIEVENTS_H

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
		bool notify, bool persistent, double expiry, const MessageOrigin::Ptr& origin);
	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);

	static void CommentAddedHandler(const Comment::Ptr& comment);
	static void CommentRemovedHandler(const Comment::Ptr& comment);

	static void DowntimeAddedHandler(const Downtime::Ptr& downtime);
	static void DowntimeRemovedHandler(const Downtime::Ptr& downtime);
	static void DowntimeStartedHandler(const Downtime::Ptr& downtime);
	static void DowntimeTriggeredHandler(const Downtime::Ptr& downtime);
};

}

#endif /* APIEVENTS_H */
