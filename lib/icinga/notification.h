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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "icinga/i2-icinga.h"
#include "icinga/user.h"
#include "icinga/usergroup.h"
#include "icinga/timeperiod.h"
#include "base/array.h"

namespace icinga
{

/**
 * The notification type.
 *
 * @ingroup icinga
 */
enum NotificationType
{
	NotificationDowntimeStart = 0,
	NotificationDowntimeEnd = 1,
	NotificationDowntimeRemoved = 2,
	NotificationCustom = 3,
	NotificationAcknowledgement = 4,
	NotificationProblem = 5,
	NotificationRecovery = 6,
	NotificationFlappingStart = 7 ,
	NotificationFlappingEnd = 8,
};

class Service;
class NotificationCommand;

/**
 * An Icinga notification specification.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Notification : public DynamicObject, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(Notification);
	DECLARE_TYPENAME(Notification);

	shared_ptr<Service> GetService(void) const;
	shared_ptr<NotificationCommand> GetNotificationCommand(void) const;
	double GetNotificationInterval(void) const;
	TimePeriod::Ptr GetNotificationPeriod(void) const;
	Dictionary::Ptr GetMacros(void) const;
	Array::Ptr GetExportMacros(void) const;
	std::set<User::Ptr> GetUsers(void) const;
	std::set<UserGroup::Ptr> GetUserGroups(void) const;
	Dictionary::Ptr GetTimes(void) const;
 	unsigned long GetNotificationTypeFilter(void) const;
	unsigned long GetNotificationStateFilter(void) const;

	double GetLastNotification(void) const;
	void SetLastNotification(double time);

	double GetNextNotification(void) const;
	void SetNextNotification(double time, const String& authority = String());

	long GetNotificationNumber(void) const;
	void UpdateNotificationNumber(void);
	void ResetNotificationNumber(void);

	void BeginExecuteNotification(NotificationType type, const Dictionary::Ptr& cr, bool force, const String& author = "", const String& text = "");

	static String NotificationTypeToString(NotificationType type);

	virtual bool ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const;

	static boost::signals2::signal<void (const Notification::Ptr&, double, const String&)> OnNextNotificationChanged;

protected:
	virtual void Start(void);
	virtual void Stop(void);

	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	String m_NotificationCommand;
	Value m_NotificationInterval;
	String m_NotificationPeriod;
	Value m_LastNotification;
	Value m_NextNotification;
	Value m_NotificationNumber;
	Dictionary::Ptr m_Macros;
	Array::Ptr m_ExportMacros;
	Array::Ptr m_Users;
	Array::Ptr m_UserGroups;
	Dictionary::Ptr m_Times;
	Value m_NotificationTypeFilter;
	Value m_NotificationStateFilter;
	String m_HostName;
	String m_Service;

	void ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const Dictionary::Ptr& cr, bool force, const String& author = "", const String& text = "");
};

}

#endif /* NOTIFICATION_H */
