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

#include "icinga/user.h"
#include "icinga/usergroup.h"
#include "base/dynamictype.h"
#include "base/utility.h"
#include "base/objectlock.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_TYPE(User);

void User::OnConfigLoaded(void)
{
	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->AddMember(GetSelf());
		}
	}
}

void User::Stop(void)
{
	DynamicObject::Stop();

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->RemoveMember(GetSelf());
		}
	}
}

String User::GetDisplayName(void) const
{
	if (!m_DisplayName.IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

Array::Ptr User::GetGroups(void) const
{
	return m_Groups;
}

Dictionary::Ptr User::GetMacros(void) const
{
	return m_Macros;
}

bool User::GetEnableNotifications(void) const
{
	if (m_EnableNotifications.IsEmpty())
		return true;
	else
		return m_EnableNotifications;
}

void User::SetEnableNotifications(bool enabled)
{
	m_EnableNotifications = enabled;
}

TimePeriod::Ptr User::GetNotificationPeriod(void) const
{
	return TimePeriod::GetByName(m_NotificationPeriod);
}

unsigned long User::GetNotificationTypeFilter(void) const
{
	if (m_NotificationTypeFilter.IsEmpty())
		return ~(unsigned long)0; /* All types. */
	else
		return m_NotificationTypeFilter;
}

unsigned long User::GetNotificationStateFilter(void) const
{
	if (m_NotificationStateFilter.IsEmpty())
		return ~(unsigned long)0; /* All states. */
	else
		return m_NotificationStateFilter;
}

void User::SetLastNotification(double ts)
{
	m_LastNotification = ts;
}

double User::GetLastNotification(void) const
{
	return m_LastNotification;
}

bool User::ResolveMacro(const String& macro, const Dictionary::Ptr&, String *result) const
{
	if (macro == "USERNAME" || macro == "CONTACTNAME") {
		*result = GetName();
		return true;
	} else if (macro == "USERDISPLAYNAME" || macro == "CONTACTALIAS") {
		*result = GetDisplayName();
		return true;
	} else {
		String tmacro;

		if (macro == "USEREMAIL" || macro == "CONTACTEMAIL")
			tmacro = "email";
		else if (macro == "USERPAGER" || macro == "CONTACTPAGER")
			tmacro = "pager";
		else
			tmacro = macro;

		Dictionary::Ptr macros = GetMacros();

		if (macros && macros->Contains(tmacro)) {
			*result = macros->Get(tmacro);
			return true;
		}

		return false;
	}
}

void User::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("display_name", m_DisplayName);
		bag->Set("macros", m_Macros);
		bag->Set("groups", m_Groups);
		bag->Set("notification_period", m_NotificationPeriod);
		bag->Set("notification_type_filter", m_NotificationTypeFilter);
		bag->Set("notification_state_filter", m_NotificationStateFilter);
	}

	if (attributeTypes & Attribute_State) {
		bag->Set("enable_notifications", m_EnableNotifications);
		bag->Set("last_notification", m_LastNotification);
	}
}

void User::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_DisplayName = bag->Get("display_name");
		m_Macros = bag->Get("macros");
		m_Groups = bag->Get("groups");
		m_NotificationPeriod = bag->Get("notification_period");
		m_NotificationTypeFilter = bag->Get("notification_type_filter");
		m_NotificationStateFilter = bag->Get("notification_state_filter");
	}

	if (attributeTypes & Attribute_State) {
		m_EnableNotifications = bag->Get("enable_notifications");
		m_LastNotification = bag->Get("last_notification");
	}
}
