// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
