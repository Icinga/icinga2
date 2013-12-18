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

TimePeriod::Ptr User::GetNotificationPeriod(void) const
{
	return TimePeriod::GetByName(GetNotificationPeriodRaw());
}

bool User::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	if (macro == "USERNAME" || macro == "CONTACTNAME") {
		*result = GetName();
		return true;
	} else if (macro == "USERDISPLAYNAME" || macro == "CONTACTALIAS") {
		*result = GetDisplayName();
		return true;
	} else if (macro.SubStr(0, 5) == "_USER") {
		Dictionary::Ptr custom = GetCustom();
		*result = custom ? custom->Get(macro.SubStr(5)) : "";
		return true;
	} else if (macro.SubStr(0, 8) == "_CONTACT") {
		Dictionary::Ptr custom = GetCustom();
		*result = custom ? custom->Get(macro.SubStr(8)) : "";
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
