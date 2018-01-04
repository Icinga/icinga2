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
