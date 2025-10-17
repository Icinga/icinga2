/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SERVICESTABLE_H
#define SERVICESTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class ServicesTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(ServicesTable);

	ServicesTable(LivestatusGroupByType type = LivestatusGroupByNone);

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ServiceGroupAccessor(LivestatusGroupByType groupByType, const Object::Ptr& groupByObject);
	static Object::Ptr HostGroupAccessor(LivestatusGroupByType groupByType, const Object::Ptr& groupByObject);

	static Value ShortNameAccessor(const Value& row);
	static Value DisplayNameAccessor(const Value& row);
	static Value CheckCommandAccessor(const Value& row);
	static Value CheckCommandExpandedAccessor(const Value& row);
	static Value EventHandlerAccessor(const Value& row);
	static Value PluginOutputAccessor(const Value& row);
	static Value LongPluginOutputAccessor(const Value& row);
	static Value PerfDataAccessor(const Value& row);
	static Value CheckPeriodAccessor(const Value& row);
	static Value NotesAccessor(const Value& row);
	static Value NotesExpandedAccessor(const Value& row);
	static Value NotesUrlAccessor(const Value& row);
	static Value NotesUrlExpandedAccessor(const Value& row);
	static Value ActionUrlAccessor(const Value& row);
	static Value ActionUrlExpandedAccessor(const Value& row);
	static Value IconImageAccessor(const Value& row);
	static Value IconImageExpandedAccessor(const Value& row);
	static Value IconImageAltAccessor(const Value& row);
	static Value MaxCheckAttemptsAccessor(const Value& row);
	static Value CurrentAttemptAccessor(const Value& row);
	static Value StateAccessor(const Value& row);
	static Value HasBeenCheckedAccessor(const Value& row);
	static Value LastStateAccessor(const Value& row);
	static Value LastHardStateAccessor(const Value& row);
	static Value StateTypeAccessor(const Value& row);
	static Value CheckTypeAccessor(const Value& row);
	static Value AcknowledgedAccessor(const Value& row);
	static Value AcknowledgementTypeAccessor(const Value& row);
	static Value NoMoreNotificationsAccessor(const Value& row);
	static Value LastTimeOkAccessor(const Value& row);
	static Value LastTimeWarningAccessor(const Value& row);
	static Value LastTimeCriticalAccessor(const Value& row);
	static Value LastTimeUnknownAccessor(const Value& row);
	static Value LastCheckAccessor(const Value& row);
	static Value NextCheckAccessor(const Value& row);
	static Value LastNotificationAccessor(const Value& row);
	static Value NextNotificationAccessor(const Value& row);
	static Value CurrentNotificationNumberAccessor(const Value& row);
	static Value LastStateChangeAccessor(const Value& row);
	static Value LastHardStateChangeAccessor(const Value& row);
	static Value ScheduledDowntimeDepthAccessor(const Value& row);
	static Value IsFlappingAccessor(const Value& row);
	static Value ChecksEnabledAccessor(const Value& row);
	static Value AcceptPassiveChecksAccessor(const Value& row);
	static Value EventHandlerEnabledAccessor(const Value& row);
	static Value NotificationsEnabledAccessor(const Value& row);
	static Value ProcessPerformanceDataAccessor(const Value& row);
	static Value ActiveChecksEnabledAccessor(const Value& row);
	static Value FlapDetectionEnabledAccessor(const Value& row);
	static Value StalenessAccessor(const Value& row);
	static Value CheckIntervalAccessor(const Value& row);
	static Value RetryIntervalAccessor(const Value& row);
	static Value NotificationIntervalAccessor(const Value& row);
	static Value LowFlapThresholdAccessor(const Value& row);
	static Value HighFlapThresholdAccessor(const Value& row);
	static Value LatencyAccessor(const Value& row);
	static Value ExecutionTimeAccessor(const Value& row);
	static Value PercentStateChangeAccessor(const Value& row);
	static Value InCheckPeriodAccessor(const Value& row);
	static Value InNotificationPeriodAccessor(const Value& row);
	static Value ContactsAccessor(const Value& row);
	static Value DowntimesAccessor(const Value& row);
	static Value DowntimesWithInfoAccessor(const Value& row);
	static Value CommentsAccessor(const Value& row);
	static Value CommentsWithInfoAccessor(const Value& row);
	static Value CommentsWithExtraInfoAccessor(const Value& row);
	static Value CustomVariableNamesAccessor(const Value& row);
	static Value CustomVariableValuesAccessor(const Value& row);
	static Value CustomVariablesAccessor(const Value& row);
	static Value GroupsAccessor(const Value& row);
	static Value ContactGroupsAccessor(const Value& row);
	static Value CheckSourceAccessor(const Value& row);
	static Value IsReachableAccessor(const Value& row);
	static Value CVIsJsonAccessor(const Value& row);
	static Value OriginalAttributesAccessor(const Value& row);
};

}

#endif /* SERVICESTABLE_H */
