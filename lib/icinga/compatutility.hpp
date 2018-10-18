/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef COMPATUTILITY_H
#define COMPATUTILITY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/host.hpp"
#include "icinga/command.hpp"

namespace icinga
{

/**
 * Compatibility utility functions.
 *
 * @ingroup icinga
 */
class CompatUtility
{
public:
	/* command */
	static String GetCommandLine(const Command::Ptr& command);
	static String GetCommandName(const Command::Ptr& command);

	/* service */
	static String GetCheckableCommandArgs(const Checkable::Ptr& checkable);

	/* notification */
	static int GetCheckableNotificationsEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationLastNotification(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationNextNotification(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationNotificationNumber(const Checkable::Ptr& checkable);
	static double GetCheckableNotificationNotificationInterval(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationTypeFilter(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationStateFilter(const Checkable::Ptr& checkable);

	static std::set<User::Ptr> GetCheckableNotificationUsers(const Checkable::Ptr& checkable);
	static std::set<UserGroup::Ptr> GetCheckableNotificationUserGroups(const Checkable::Ptr& checkable);

	/* check result */
	static String GetCheckResultOutput(const CheckResult::Ptr& cr);
	static String GetCheckResultLongOutput(const CheckResult::Ptr& cr);

	/* misc */
	static String EscapeString(const String& str);
	static String UnEscapeString(const String& str);

private:
	CompatUtility();

	static String GetCommandNamePrefix(const Command::Ptr& command);
};

}

#endif /* COMPATUTILITY_H */
