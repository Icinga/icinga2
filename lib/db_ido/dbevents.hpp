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

#ifndef DBEVENTS_H
#define DBEVENTS_H

#include "db_ido/dbobject.hpp"
#include "base/dynamicobject.hpp"
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

enum EnableType
{
    EnableActiveChecks = 1,
    EnablePassiveChecks = 2,
    EnableNotifications = 3,
    EnablePerfdata = 4,
    EnableFlapping = 5
};

/**
 * IDO events
 *
 * @ingroup ido
 */
class DbEvents
{
public:
	static void StaticInitialize(void);

	static void AddCommentByType(const DynamicObject::Ptr& object, const Comment::Ptr& comment, bool historical);
	static void AddComments(const Checkable::Ptr& checkable);
	static void RemoveComments(const Checkable::Ptr& checkable);

	static void AddDowntimeByType(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, bool historical);
	static void AddDowntimes(const Checkable::Ptr& checkable);
	static void RemoveDowntimes(const Checkable::Ptr& checkable);

	static void AddLogHistory(const Checkable::Ptr& checkable, String buffer, LogEntryType type);

	/* Status */
	static void NextCheckChangedHandler(const Checkable::Ptr& checkable, double nextCheck);
	static void FlappingChangedHandler(const Checkable::Ptr& checkable, FlappingState state);
	static void LastNotificationChangedHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable);

	static void EnableActiveChecksChangedHandler(const Checkable::Ptr& checkable, bool enabled);
	static void EnablePassiveChecksChangedHandler(const Checkable::Ptr& checkable, bool enabled);
	static void EnableNotificationsChangedHandler(const Checkable::Ptr& checkable, bool enabled);
	static void EnablePerfdataChangedHandler(const Checkable::Ptr& checkable, bool enabled);
	static void EnableFlappingChangedHandler(const Checkable::Ptr& checkable, bool enabled);

	static void AddComment(const Checkable::Ptr& checkable, const Comment::Ptr& comment);
	static void RemoveComment(const Checkable::Ptr& checkable, const Comment::Ptr& comment);

	static void AddDowntime(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, bool remove_existing);
	static void RemoveDowntime(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime);
	static void TriggerDowntime(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime);

	static void AddAcknowledgement(const Checkable::Ptr& checkable, AcknowledgementType type);
	static void RemoveAcknowledgement(const Checkable::Ptr& checkable);
	static void AddAcknowledgementInternal(const Checkable::Ptr& checkable, AcknowledgementType type, bool add);

	static void ReachabilityChangedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, std::set<Checkable::Ptr> children);

	/* comment, downtime, acknowledgement history */
	static void AddCommentHistory(const Checkable::Ptr& checkable, const Comment::Ptr& comment);
	static void AddDowntimeHistory(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime);
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
	static void AddTriggerDowntimeLogHistory(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime);
	static void AddRemoveDowntimeLogHistory(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime);
	static void AddNotificationSentLogHistory(const Notification::Ptr& notification, const Checkable::Ptr& checkable,
	    const User::Ptr& user, NotificationType notification_type, const CheckResult::Ptr& cr, const String& author,
	    const String& comment_text);
	static void AddFlappingLogHistory(const Checkable::Ptr& checkable, FlappingState flapping_state);

	/* other history */
	static void AddFlappingHistory(const Checkable::Ptr& checkable, FlappingState flapping_state);
	static void AddServiceCheckHistory(const Checkable::Ptr& checkable, const CheckResult::Ptr &cr);
	static void AddEventHandlerHistory(const Checkable::Ptr& checkable);
	static void AddExternalCommandHistory(double time, const String& command, const std::vector<String>& arguments);

private:
	DbEvents(void);

	static void AddCommentInternal(const Checkable::Ptr& checkable, const Comment::Ptr& comment, bool historical);
	static void AddDowntimeInternal(const Checkable::Ptr& checkable, const Downtime::Ptr& downtime, bool historical);
	static void EnableChangedHandlerInternal(const Checkable::Ptr& checkable, bool enabled, EnableType type);
};

}

#endif /* DBEVENTS_H */
