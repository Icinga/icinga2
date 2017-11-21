/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "livestatus/downtimestable.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicestable.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include <boost/tuple/tuple.hpp>

using namespace icinga;

DowntimesTable::DowntimesTable(void)
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
	ServicesTable::AddColumns(table, "service_", std::bind(&DowntimesTable::ServiceAccessor, _1, objectAccessor));
	HostsTable::AddColumns(table, "host_", std::bind(&DowntimesTable::HostAccessor, _1, objectAccessor));
}

String DowntimesTable::GetName(void) const
{
	return "downtimes";
}

String DowntimesTable::GetPrefix(void) const
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
