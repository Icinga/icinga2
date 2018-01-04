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

#ifndef COMPATUTILITY_H
#define COMPATUTILITY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/host.hpp"
#include "icinga/command.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include <vector>

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
	static String GetCommandName(const Command::Ptr command);

	/* host */
	static int GetHostCurrentState(const Host::Ptr& host);
	static String GetHostStateString(const Host::Ptr& host);
	static String GetHostAlias(const Host::Ptr& host);
	static int GetHostNotifyOnDown(const Host::Ptr& host);
	static int GetHostNotifyOnUnreachable(const Host::Ptr& host);

	/* service */
	static String GetCheckableCommandArgs(const Checkable::Ptr& checkable);
	static int GetCheckableCheckType(const Checkable::Ptr& checkable);
	static double GetCheckableCheckInterval(const Checkable::Ptr& checkable);
	static double GetCheckableRetryInterval(const Checkable::Ptr& checkable);
	static String GetCheckableCheckPeriod(const Checkable::Ptr& checkable);
	static int GetCheckableHasBeenChecked(const Checkable::Ptr& checkable);
	static int GetCheckableProblemHasBeenAcknowledged(const Checkable::Ptr& checkable);
	static int GetCheckableAcknowledgementType(const Checkable::Ptr& checkable);
	static int GetCheckablePassiveChecksEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableActiveChecksEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableEventHandlerEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableFlapDetectionEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableIsFlapping(const Checkable::Ptr& checkable);
	static int GetCheckableIsReachable(const Checkable::Ptr& checkable);
	static double GetCheckablePercentStateChange(const Checkable::Ptr& checkable);
	static int GetCheckableProcessPerformanceData(const Checkable::Ptr& checkable);

	static String GetCheckableEventHandler(const Checkable::Ptr& checkable);
	static String GetCheckableCheckCommand(const Checkable::Ptr& checkable);

	static int GetCheckableIsVolatile(const Checkable::Ptr& checkable);
	static double GetCheckableLowFlapThreshold(const Checkable::Ptr& checkable);
	static double GetCheckableHighFlapThreshold(const Checkable::Ptr& checkable);
	static int GetCheckableFreshnessChecksEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableFreshnessThreshold(const Checkable::Ptr& checkable);
	static double GetCheckableStaleness(const Checkable::Ptr& checkable);
	static int GetCheckableIsAcknowledged(const Checkable::Ptr& checkable);
	static int GetCheckableNoMoreNotifications(const Checkable::Ptr& checkable);
	static int GetCheckableInCheckPeriod(const Checkable::Ptr& checkable);
	static int GetCheckableInNotificationPeriod(const Checkable::Ptr& checkable);

	/* notification */
	static int GetCheckableNotificationsEnabled(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationLastNotification(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationNextNotification(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationNotificationNumber(const Checkable::Ptr& checkable);
	static double GetCheckableNotificationNotificationInterval(const Checkable::Ptr& checkable);
	static String GetCheckableNotificationNotificationOptions(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationTypeFilter(const Checkable::Ptr& checkable);
	static int GetCheckableNotificationStateFilter(const Checkable::Ptr& checkable);
	static int GetCheckableNotifyOnWarning(const Checkable::Ptr& checkable);
	static int GetCheckableNotifyOnCritical(const Checkable::Ptr& checkable);
	static int GetCheckableNotifyOnUnknown(const Checkable::Ptr& checkable);
	static int GetCheckableNotifyOnRecovery(const Checkable::Ptr& checkable);
	static int GetCheckableNotifyOnFlapping(const Checkable::Ptr& checkable);
	static int GetCheckableNotifyOnDowntime(const Checkable::Ptr& checkable);

	static std::set<User::Ptr> GetCheckableNotificationUsers(const Checkable::Ptr& checkable);
	static std::set<UserGroup::Ptr> GetCheckableNotificationUserGroups(const Checkable::Ptr& checkable);

	/* custom attribute */
	static String GetCustomAttributeConfig(const CustomVarObject::Ptr& object, const String& name);
	static Dictionary::Ptr GetCustomAttributeConfig(const CustomVarObject::Ptr& object);

	/* check result */
	static String GetCheckResultOutput(const CheckResult::Ptr& cr);
	static String GetCheckResultLongOutput(const CheckResult::Ptr& cr);
	static String GetCheckResultPerfdata(const CheckResult::Ptr& cr);

	/* misc */
	static std::pair<unsigned long, unsigned long> ConvertTimestamp(double time);

	static int MapNotificationReasonType(NotificationType type);
	static int MapExternalCommandType(const String& name);

	static String EscapeString(const String& str);
	static String UnEscapeString(const String& str);

private:
	CompatUtility();

	static String GetCommandNamePrefix(const Command::Ptr command);
};

}

#endif /* COMPATUTILITY_H */
