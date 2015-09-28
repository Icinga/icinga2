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

#ifndef APIEVENTS_H
#define APIEVENTS_H

#include "icinga/checkable.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class I2_ICINGA_API ApiEvents
{
public:
	static void StaticInitialize(void);

	static void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin);
	static Value CheckResultAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void NextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value NextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void NextNotificationChangedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin);
	static Value NextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void ForceNextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value ForceNextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void ForceNextNotificationChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value ForceNextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void CommentAddedHandler(const Checkable::Ptr& checkable, const Comment::Ptr& comment, const MessageOrigin::Ptr& origin);
	static Value CommentAddedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void CommentRemovedHandler(const Checkable::Ptr& checkable, const Comment::Ptr& comment, const MessageOrigin::Ptr& origin);
	static Value CommentRemovedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void DowntimeAddedHandler(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, const MessageOrigin::Ptr& origin);
	static Value DowntimeAddedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void DowntimeRemovedHandler(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, const MessageOrigin::Ptr& origin);
	static Value DowntimeRemovedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void AcknowledgementSetHandler(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type,
	    bool notify, double expiry, const MessageOrigin::Ptr& origin);
	static Value AcknowledgementSetAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value AcknowledgementClearedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static Value ExecuteCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static String GetRepositoryDir(void);
	static void RepositoryTimerHandler(void);
	static Value UpdateRepositoryAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static Dictionary::Ptr MakeCheckResultMessage(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
};

}

#endif /* APIEVENTS_H */
