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

#ifndef COMPATUTILITY_H
#define COMPATUTILITY_H

#include "icinga/i2-icinga.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "base/dictionary.h"
#include "base/dynamicobject.h"
#include <vector>

namespace icinga
{

/**
 * @ingroup icinga
 */
enum CompatObjectType
{
	CompatTypeService,
	CompatTypeHost
};

typedef struct {
	int have_2d_coords;
	String x_2d;
	String y_2d;
} Host2dCoords;

/**
 * Compatibility utility functions.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API CompatUtility
{
public:

	/* host */
	static String GetHostAlias(const Host::Ptr& host);
	static String GetHostAddress(const Host::Ptr& host);
	static String GetHostAddress6(const Host::Ptr& host);
	static Host2dCoords GetHost2dCoords(const Host::Ptr& host);
	static int GetHostNotifyOnDown(const Host::Ptr& host);
	static int GetHostNotifyOnUnreachable(const Host::Ptr& host);

	/* service */
	static int GetServiceCurrentState(const Service::Ptr& service);
	static int GetServiceShouldBeScheduled(const Service::Ptr& service);
	static int GetServiceCheckType(const Service::Ptr& service);
	static double GetServiceCheckInterval(const Service::Ptr& service);
	static double GetServiceRetryInterval(const Service::Ptr& service);
	static String GetServiceCheckPeriod(const Service::Ptr& service);
	static int GetServiceHasBeenChecked(const Service::Ptr& service);
	static int GetServiceProblemHasBeenAcknowledged(const Service::Ptr& service);
	static int GetServiceAcknowledgementType(const Service::Ptr& service);
	static int GetServicePassiveChecksEnabled(const Service::Ptr& service);
	static int GetServiceActiveChecksEnabled(const Service::Ptr& service);
	static int GetServiceEventHandlerEnabled(const Service::Ptr& service);
	static int GetServiceFlapDetectionEnabled(const Service::Ptr& service);
	static int GetServiceIsFlapping(const Service::Ptr& service);
	static String GetServicePercentStateChange(const Service::Ptr& service);
	static int GetServiceProcessPerformanceData(const Service::Ptr& service);

	static String GetServiceEventHandler(const Service::Ptr& service);
	static String GetServiceCheckCommand(const Service::Ptr& service);

	static int GetServiceIsVolatile(const Service::Ptr& service);
	static double GetServiceLowFlapThreshold(const Service::Ptr& service);
	static double GetServiceHighFlapThreshold(const Service::Ptr& service);
	static int GetServiceFreshnessChecksEnabled(const Service::Ptr& service);
	static int GetServiceFreshnessThreshold(const Service::Ptr& service);
	static double GetServiceStaleness(const Service::Ptr& service);
	static int GetServiceIsAcknowledged(const Service::Ptr& service);
	static int GetServiceNoMoreNotifications(const Service::Ptr& service);
	static int GetServiceInCheckPeriod(const Service::Ptr& service);
	static int GetServiceInNotificationPeriod(const Service::Ptr& service);

	/* notification */
	static int GetServiceNotificationsEnabled(const Service::Ptr& service);
	static int GetServiceNotificationLastNotification(const Service::Ptr& service);
	static int GetServiceNotificationNextNotification(const Service::Ptr& service);
	static int GetServiceNotificationNotificationNumber(const Service::Ptr& service);
	static double GetServiceNotificationNotificationInterval(const Service::Ptr& service);
	static String GetServiceNotificationNotificationPeriod(const Service::Ptr& service);
	static String GetServiceNotificationNotificationOptions(const Service::Ptr& service);
	static int GetServiceNotificationTypeFilter(const Service::Ptr& service);
	static int GetServiceNotificationStateFilter(const Service::Ptr& service);
	static int GetServiceNotifyOnWarning(const Service::Ptr& service);
	static int GetServiceNotifyOnCritical(const Service::Ptr& service);
	static int GetServiceNotifyOnUnknown(const Service::Ptr& service);
	static int GetServiceNotifyOnRecovery(const Service::Ptr& service);
	static int GetServiceNotifyOnFlapping(const Service::Ptr& service);
	static int GetServiceNotifyOnDowntime(const Service::Ptr& service);

	static std::set<User::Ptr> GetServiceNotificationUsers(const Service::Ptr& service);
	static std::set<UserGroup::Ptr> GetServiceNotificationUserGroups(const Service::Ptr& service);

	/* command */
	static String GetCommandLine(const Command::Ptr& command);

	/* custom attribute */
	static String GetCustomAttributeConfig(const DynamicObject::Ptr& object, const String& name);
	static Dictionary::Ptr GetCustomVariableConfig(const DynamicObject::Ptr& object);

	/* check result */
	static String GetCheckResultOutput(const CheckResult::Ptr& cr);
	static String GetCheckResultLongOutput(const CheckResult::Ptr& cr);
	static String GetCheckResultPerfdata(const CheckResult::Ptr& cr);

	/* misc */
	static std::pair<unsigned long, unsigned long> ConvertTimestamp(double time);

	static int MapNotificationReasonType(NotificationType type);
	static int MapExternalCommandType(const String& name);

	static String EscapeString(const String& str);

private:
	CompatUtility(void);
};

}

#endif /* COMPATUTILITY_H */
