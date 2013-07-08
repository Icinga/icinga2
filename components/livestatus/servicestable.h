/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef SERVICESTABLE_H
#define SERVICESTABLE_H

#include "livestatus/table.h"

using namespace icinga;

namespace livestatus
{

/**
 * @ingroup livestatus
 */
class ServicesTable : public Table
{
public:
	typedef shared_ptr<ServicesTable> Ptr;
	typedef weak_ptr<ServicesTable> WeakPtr;

	ServicesTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn);

	static Object::Ptr HostAccessor(const Object::Ptr& object);

	static Value ShortNameAccessor(const Object::Ptr& object);
	static Value DisplayNameAccessor(const Object::Ptr& object);
	static Value CheckCommandAccessor(const Object::Ptr& object);
	static Value CheckCommandExpandedAccessor(const Object::Ptr& object);
	static Value EventHandlerAccessor(const Object::Ptr& object);
	static Value PluginOutputAccessor(const Object::Ptr& object);
	static Value LongPluginOutputAccessor(const Object::Ptr& object);
	static Value PerfDataAccessor(const Object::Ptr& object);
	static Value NotificationPeriodAccessor(const Object::Ptr& object);
	static Value CheckPeriodAccessor(const Object::Ptr& object);
	static Value NotesAccessor(const Object::Ptr& object);
	static Value NotesExpandedAccessor(const Object::Ptr& object);
	static Value NotesUrlAccessor(const Object::Ptr& object);
	static Value NotesUrlExpandedAccessor(const Object::Ptr& object);
	static Value ActionUrlAccessor(const Object::Ptr& object);
	static Value ActionUrlExpandedAccessor(const Object::Ptr& object);
	static Value IconImageAccessor(const Object::Ptr& object);
	static Value IconImageExpandedAccessor(const Object::Ptr& object);
	static Value IconImageAltAccessor(const Object::Ptr& object);
	static Value InitialStateAccessor(const Object::Ptr& object);
	static Value MaxCheckAttemptsAccessor(const Object::Ptr& object);
	static Value CurrentAttemptAccessor(const Object::Ptr& object);
	static Value StateAccessor(const Object::Ptr& object);
	static Value HasBeenCheckedAccessor(const Object::Ptr& object);
	static Value LastStateAccessor(const Object::Ptr& object);
	static Value LastHardStateAccessor(const Object::Ptr& object);
	static Value StateTypeAccessor(const Object::Ptr& object);
	static Value CheckTypeAccessor(const Object::Ptr& object);
	static Value AcknowledgedAccessor(const Object::Ptr& object);
	static Value AcknowledgementTypeAccessor(const Object::Ptr& object);
	static Value NoMoreNotificationsAccessor(const Object::Ptr& object);
	static Value LastTimeOkAccessor(const Object::Ptr& object);
	static Value LastTimeWarningAccessor(const Object::Ptr& object);
	static Value LastTimeCriticalAccessor(const Object::Ptr& object);
	static Value LastTimeUnknownAccessor(const Object::Ptr& object);
	static Value LastCheckAccessor(const Object::Ptr& object);
	static Value NextCheckAccessor(const Object::Ptr& object);
	static Value LastNotificationAccessor(const Object::Ptr& object);
	static Value NextNotificationAccessor(const Object::Ptr& object);
	static Value CurrentNotificationNumberAccessor(const Object::Ptr& object);
	static Value LastStateChangeAccessor(const Object::Ptr& object);
	static Value LastHardStateChangeAccessor(const Object::Ptr& object);
	static Value ScheduledDowntimeDepthAccessor(const Object::Ptr& object);
	static Value IsFlappingAccessor(const Object::Ptr& object);
	static Value ChecksEnabledAccessor(const Object::Ptr& object);
	static Value AcceptPassiveChecksAccessor(const Object::Ptr& object);
	static Value EventHandlerEnabledAccessor(const Object::Ptr& object);
	static Value NotificationsEnabledAccessor(const Object::Ptr& object);
	static Value ProcessPerformanceDataAccessor(const Object::Ptr& object);
	static Value IsExecutingAccessor(const Object::Ptr& object);
	static Value ActiveChecksEnabledAccessor(const Object::Ptr& object);
	static Value CheckOptionsAccessor(const Object::Ptr& object);
	static Value FlapDetectionEnabledAccessor(const Object::Ptr& object);
	static Value CheckFreshnessAccessor(const Object::Ptr& object);
	static Value ObsessOverServiceAccessor(const Object::Ptr& object);
	static Value ModifiedAttributesAccessor(const Object::Ptr& object);
	static Value ModifiedAttributesListAccessor(const Object::Ptr& object);
	static Value PnpgraphPresentAccessor(const Object::Ptr& object);
	static Value CheckIntervalAccessor(const Object::Ptr& object);
	static Value RetryIntervalAccessor(const Object::Ptr& object);
	static Value NotificationIntervalAccessor(const Object::Ptr& object);
	static Value FirstNotificationDelayAccessor(const Object::Ptr& object);
	static Value LowFlapThresholdAccessor(const Object::Ptr& object);
	static Value HighFlapThresholdAccessor(const Object::Ptr& object);
	static Value LatencyAccessor(const Object::Ptr& object);
	static Value ExecutionTimeAccessor(const Object::Ptr& object);
	static Value PercentStateChangeAccessor(const Object::Ptr& object);
	static Value InCheckPeriodAccessor(const Object::Ptr& object);
	static Value InNotificationPeriodAccessor(const Object::Ptr& object);
	static Value ContactsAccessor(const Object::Ptr& object);
	static Value DowntimesAccessor(const Object::Ptr& object);
	static Value DowntimesWithInfoAccessor(const Object::Ptr& object);
	static Value CommentsAccessor(const Object::Ptr& object);
	static Value CommentsWithInfoAccessor(const Object::Ptr& object);
	static Value CommentsWithExtraInfoAccessor(const Object::Ptr& object);
	static Value CustomVariableNamesAccessor(const Object::Ptr& object);
	static Value CustomVariableValuesAccessor(const Object::Ptr& object);
	static Value CustomVariablesAccessor(const Object::Ptr& object);
	static Value GroupsAccessor(const Object::Ptr& object);
	static Value ContactGroupsAccessor(const Object::Ptr& object);
};

}

#endif /* SERVICESTABLE_H */
