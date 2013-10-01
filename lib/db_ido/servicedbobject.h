/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef SERVICEDBOBJECT_H
#define SERVICEDBOBJECT_H

#include "db_ido/dbobject.h"
#include "base/dynamicobject.h"
#include "icinga/service.h"

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
 * A Service database object.
 *
 * @ingroup ido
 */
class ServiceDbObject : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(ServiceDbObject);

	ServiceDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

	static void StaticInitialize(void);

	virtual Dictionary::Ptr GetConfigFields(void) const;
	virtual Dictionary::Ptr GetStatusFields(void) const;

protected:
	virtual bool IsStatusAttribute(const String& attribute) const;

	virtual void OnConfigUpdate(void);
	virtual void OnStatusUpdate(void);

private:
        static void AddCommentInternal(const Service::Ptr& service, const Dictionary::Ptr& comment, bool historical);
        static void AddCommentByType(const DynamicObject::Ptr& object, const Dictionary::Ptr& comment, bool historical);
        static void AddComments(const Service::Ptr& service);
        static void RemoveComments(const Service::Ptr& service);

        static void AddDowntimeInternal(const Service::Ptr& service, const Dictionary::Ptr& downtime, bool historical);
        static void AddDowntimeByType(const DynamicObject::Ptr& object, const Dictionary::Ptr& downtime, bool historical);
        static void AddDowntimes(const Service::Ptr& service);
        static void RemoveDowntimes(const Service::Ptr& service);

        static void AddLogHistory(const Service::Ptr& service, String buffer, LogEntryType type);

        /* Status */
	static void AddComment(const Service::Ptr& service, const Dictionary::Ptr& comment);
	static void RemoveComment(const Service::Ptr& service, const Dictionary::Ptr& comment);

	static void AddDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime);
	static void RemoveDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime);
	static void TriggerDowntime(const Service::Ptr& service, const Dictionary::Ptr& downtime);

        /* History */
        static void AddCommentHistory(const Service::Ptr& service, const Dictionary::Ptr& comment);
        static void AddDowntimeHistory(const Service::Ptr& service, const Dictionary::Ptr& downtime);
        static void AddAcknowledgementHistory(const Service::Ptr& service, const String& author, const String& comment, AcknowledgementType type, double expiry);
        static void AddContactNotificationHistory(const Service::Ptr& service, const User::Ptr& user);
        static void AddNotificationHistory(const Service::Ptr& service, const std::set<User::Ptr>& users, NotificationType type, const Dictionary::Ptr& cr, const String& author, const String& text);
        static void AddStateChangeHistory(const Service::Ptr& service, const Dictionary::Ptr& cr, StateType type);

        static void AddCheckResultLogHistory(const Service::Ptr& service, const Dictionary::Ptr &cr);
        static void AddTriggerDowntimeLogHistory(const Service::Ptr& service, const Dictionary::Ptr& downtime);
        static void AddRemoveDowntimeLogHistory(const Service::Ptr& service, const Dictionary::Ptr& downtime);
        static void AddNotificationSentLogHistory(const Service::Ptr& service, const User::Ptr& user, NotificationType const& notification_type, Dictionary::Ptr const& cr, const String& author, const String& comment_text);
        static void AddFlappingLogHistory(const Service::Ptr& service, FlappingState flapping_state);

        static void AddFlappingHistory(const Service::Ptr& service, FlappingState flapping_state);
        static void AddServiceCheckHistory(const Service::Ptr& service, const Dictionary::Ptr &cr);
        static void AddEventHandlerHistory(const Service::Ptr& service);
        static void AddExternalCommandHistory(double time, const String& command, const std::vector<String>& arguments);
};

}

#endif /* SERVICEDBOBJECT_H */
