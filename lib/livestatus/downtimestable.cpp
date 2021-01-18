/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/downtimestable.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicestable.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

DowntimesTable::DowntimesTable()
{
	AddColumns(this);
}

void DowntimesTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "author", Column(&DowntimesTable::AuthorAccessor, objectAccessor));
	table->AddColumn(prefix + "comment", Column(&DowntimesTable::CommentAccessor, objectAccessor));
	table->AddColumn(prefix + "id", Column(&DowntimesTable::IdAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_time", Column(&DowntimesTable::EntryTimeAccessor, objectAccessor));
	table->AddColumn(prefix + "type", Column(&DowntimesTable::TypeAccessor, objectAccessor));
	table->AddColumn(prefix + "is_service", Column(&DowntimesTable::IsServiceAccessor, objectAccessor));
	table->AddColumn(prefix + "start_time", Column(&DowntimesTable::StartTimeAccessor, objectAccessor));
	table->AddColumn(prefix + "end_time", Column(&DowntimesTable::EndTimeAccessor, objectAccessor));
	table->AddColumn(prefix + "fixed", Column(&DowntimesTable::FixedAccessor, objectAccessor));
	table->AddColumn(prefix + "duration", Column(&DowntimesTable::DurationAccessor, objectAccessor));
	table->AddColumn(prefix + "triggered_by", Column(&DowntimesTable::TriggeredByAccessor, objectAccessor));

	/* order is important - host w/o services must not be empty */
	ServicesTable::AddColumns(table, "service_", [objectAccessor](const Value& row, LivestatusGroupByType, const Object::Ptr&) -> Value {
		return ServiceAccessor(row, objectAccessor);
	});
	HostsTable::AddColumns(table, "host_", [objectAccessor](const Value& row, LivestatusGroupByType, const Object::Ptr&) -> Value {
		return HostAccessor(row, objectAccessor);
	});
}

String DowntimesTable::GetName() const
{
	return "downtimes";
}

String DowntimesTable::GetPrefix() const
{
	return "downtime";
}

void DowntimesTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const Downtime::Ptr& downtime : ConfigType::GetObjectsByType<Downtime>()) {
		if (!addRowFn(downtime, LivestatusGroupByNone, Empty))
			return;
	}
}

Object::Ptr DowntimesTable::HostAccessor(const Value& row, const Column::ObjectAccessor&)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	Checkable::Ptr checkable = downtime->GetCheckable();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	return host;
}

Object::Ptr DowntimesTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor&)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	Checkable::Ptr checkable = downtime->GetCheckable();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	return service;
}

Value DowntimesTable::AuthorAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return downtime->GetAuthor();
}

Value DowntimesTable::CommentAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return downtime->GetComment();
}

Value DowntimesTable::IdAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return downtime->GetLegacyId();
}

Value DowntimesTable::EntryTimeAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return static_cast<int>(downtime->GetEntryTime());
}

Value DowntimesTable::TypeAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);
	// 1 .. active, 0 .. pending
	return (downtime->IsInEffect() ? 1 : 0);
}

Value DowntimesTable::IsServiceAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);
	Checkable::Ptr checkable = downtime->GetCheckable();

	return (dynamic_pointer_cast<Host>(checkable) ? 0 : 1);
}

Value DowntimesTable::StartTimeAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return static_cast<int>(downtime->GetStartTime());
}

Value DowntimesTable::EndTimeAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return static_cast<int>(downtime->GetEndTime());
}

Value DowntimesTable::FixedAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return downtime->GetFixed();
}

Value DowntimesTable::DurationAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	return downtime->GetDuration();
}

Value DowntimesTable::TriggeredByAccessor(const Value& row)
{
	Downtime::Ptr downtime = static_cast<Downtime::Ptr>(row);

	String triggerDowntimeName = downtime->GetTriggeredBy();

	Downtime::Ptr triggerDowntime = Downtime::GetByName(triggerDowntimeName);

	if (triggerDowntime)
		return triggerDowntime->GetLegacyId();

	return Empty;
}
