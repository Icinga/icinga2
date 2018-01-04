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

#ifndef DBEVENTS_H
#define DBEVENTS_H

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"
#include "icinga/service.hpp"

namespace icinga
{

enum LogEntryType
{
	LogEntryTypeRuntimeError = 1,
	LogEntryTypeRuntimeWarning = 2,
	LogEntryTypeVerificationError = 4,
	LogEntryTypeVerificationWarning = 8,
	LogEntryTypeConfigError = 16,
	LogEntryTypeConfigWarning = 32,
	LogEntryTypeProcessInfo = 64,
	LogEntryTypeEventHandler = 128,
	LogEntryTypeExternalCommand = 512,
	LogEntryTypeHostUp = 1024,
	LogEntryTypeHostDown = 2048,
	LogEntryTypeHostUnreachable = 4096,
	LogEntryTypeServiceOk = 8192,
	LogEntryTypeServiceUnknown = 16384,
	LogEntryTypeServiceWarning = 32768,
	LogEntryTypeServiceCritical = 65536,
	LogEntryTypePassiveCheck = 1231072,
	LogEntryTypeInfoMessage = 262144,
	LogEntryTypeHostNotification = 524288,
	LogEntryTypeServiceNotification = 1048576
};

/**
 * IDO events
 *
 * @ingroup ido
 */
class DbEvents
{
public:
	static void StaticInitialize();

	static void AddComments(const Checkable::Ptr& checkable);

	static void AddDowntimes(const Checkable::Ptr& checkable);
	static void RemoveDowntimes(const Checkable::Ptr& checkable);

	static void AddLogHistory(const Checkable::Ptr& checkable, const String& buffer, LogEntryType type);

	/* Status */
	static void NextCheckUpdatedHandler(const Checkable::Ptr& checkable);
	static void FlappingChangedHandler(const Checkable::Ptr& checkable);
	static void LastNotificationChangedHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable);

	static void EnableActiveChecksChangedHandler(const Checkable::Ptr& checkable);
	static void EnablePassiveChecksChangedHandler(const Checkable::Ptr& checkable);
	static void EnableNotificationsChangedHandler(const Checkable::Ptr& checkable);
	static void EnablePerfdataChangedHandler(const Checkable::Ptr& checkable);
	static void EnableFlappingChangedHandler(const Checkable::Ptr& checkable);

	static void AddComment(const Comment::Ptr& comment);
	static void RemoveComment(const Comment::Ptr& comment);

	static void AddDowntime(const Downtime::Ptr& downtime);
	static void RemoveDowntime(const Downtime::Ptr& downtime);
	static void TriggerDowntime(const Downtime::Ptr& downtime);

	static void AddAcknowledgement(const Checkable::Ptr& checkable, AcknowledgementType type);
	static void RemoveAcknowledgement(const Checkable::Ptr& checkable);
	static void AddAcknowledgementInternal(const Checkable::Ptr& checkable, AcknowledgementType type, bool add);

	static void ReachabilityChangedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, std::set<Checkable::Ptr> children);

	/* comment, downtime, acknowledgement history */
	static void AddCommentHistory(const Comment::Ptr& comment);
	static void AddDowntimeHistory(const Downtime::Ptr& downtime);
	static void AddAcknowledgementHistory(const Checkable::Ptr& checkable, const String& author, const String& comment,
		AcknowledgementType type, bool notify, double expiry);

	/* notification & contactnotification history */
	static void AddNotificationHistory(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
		const std::set<User::Ptr>& users, NotificationType type, const CheckResult::Ptr& cr, const String& author,
		const String& text);

	/* statehistory */
	static void AddStateChangeHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);

	/* logentries */
	static void AddCheckResultLogHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr &cr);
	static void AddTriggerDowntimeLogHistory(const Downtime::Ptr& downtime);
	static void AddRemoveDowntimeLogHistory(const Downtime::Ptr& downtime);
	static void AddNotificationSentLogHistory(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
		const User::Ptr& user, NotificationType notification_type, const CheckResult::Ptr& cr, const String& author,
		const String& comment_text);

	static void AddFlappingChangedLogHistory(const Checkable::Ptr& checkable);
	static void AddEnableFlappingChangedLogHistory(const Checkable::Ptr& checkable);

	/* other history */
	static void AddFlappingChangedHistory(const Checkable::Ptr& checkable);
	static void AddEnableFlappingChangedHistory(const Checkable::Ptr& checkable);
	static void AddCheckableCheckHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr &cr);
	static void AddEventHandlerHistory(const Checkable::Ptr& checkable);
	static void AddExternalCommandHistory(double time, const String& command, const std::vector<String>& arguments);

private:
	DbEvents();

	static void AddCommentInternal(std::vector<DbQuery>& queries, const Comment::Ptr& comment, bool historical);
	static void RemoveCommentInternal(std::vector<DbQuery>& queries, const Comment::Ptr& comment);
	static void AddDowntimeInternal(std::vector<DbQuery>& queries, const Downtime::Ptr& downtime, bool historical);
	static void RemoveDowntimeInternal(std::vector<DbQuery>& queries, const Downtime::Ptr& downtime);
	static void EnableChangedHandlerInternal(const Checkable::Ptr& checkable, const String& fieldName, bool enabled);
};

}

#endif /* DBEVENTS_H */
