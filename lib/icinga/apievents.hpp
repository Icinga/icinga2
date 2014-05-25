/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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
class I2_ICINGA_API ApiEvents
{
public:
	static void StaticInitialize(void);

	static void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin& origin);
	static Value CheckResultAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void NextCheckChangedHandler(const Checkable::Ptr& checkable, double nextCheck, const MessageOrigin& origin);
	static Value NextCheckChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void NextNotificationChangedHandler(const Notification::Ptr& notification, double nextCheck, const MessageOrigin& origin);
	static Value NextNotificationChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void ForceNextCheckChangedHandler(const Checkable::Ptr& checkable, bool forced, const MessageOrigin& origin);
	static Value ForceNextCheckChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void ForceNextNotificationChangedHandler(const Checkable::Ptr& checkable, bool forced, const MessageOrigin& origin);
	static Value ForceNextNotificationChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void EnableActiveChecksChangedHandler(const Checkable::Ptr& checkable, bool enabled, const MessageOrigin& origin);
	static Value EnableActiveChecksChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void EnablePassiveChecksChangedHandler(const Checkable::Ptr& checkable, bool enabled, const MessageOrigin& origin);
	static Value EnablePassiveChecksChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void EnableNotificationsChangedHandler(const Checkable::Ptr& checkable, bool enabled, const MessageOrigin& origin);
	static Value EnableNotificationsChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void EnableFlappingChangedHandler(const Checkable::Ptr& checkable, bool enabled, const MessageOrigin& origin);
	static Value EnableFlappingChangedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void CommentAddedHandler(const Checkable::Ptr& checkable, const Comment::Ptr& comment, const MessageOrigin& origin);
	static Value CommentAddedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void CommentRemovedHandler(const Checkable::Ptr& checkable, const Comment::Ptr& comment, const MessageOrigin& origin);
	static Value CommentRemovedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void DowntimeAddedHandler(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, const MessageOrigin& origin);
	static Value DowntimeAddedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void DowntimeRemovedHandler(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, const MessageOrigin& origin);
	static Value DowntimeRemovedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void AcknowledgementSetHandler(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, double expiry, const MessageOrigin& origin);
	static Value AcknowledgementSetAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const MessageOrigin& origin);
	static Value AcknowledgementClearedAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static String GetRepositoryDir(void);
	static void RepositoryTimerHandler(void);
	static Value UpdateRepositoryAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

	static String GetVirtualHostName(const Host::Ptr& host);
	static Host::Ptr FindHostByVirtualName(const String& hostName);
};

}

#endif /* APIEVENTS_H */
