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

#include "livestatus/downtimestable.h"
#include "livestatus/servicestable.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

using namespace icinga;
using namespace livestatus;

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

	ServicesTable::AddColumns(table, "service_", boost::bind(&DowntimesTable::ServiceAccessor, _1, objectAccessor));
}

String DowntimesTable::GetName(void) const
{
	return "downtimes";
}

void DowntimesTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		Dictionary::Ptr downtimes = service->GetDowntimes();

		if (!downtimes)
			continue;

		ObjectLock olock(downtimes);

		String id;
		BOOST_FOREACH(boost::tie(id, boost::tuples::ignore), downtimes) {
			if (Service::GetOwnerByDowntimeID(id) == service)
				addRowFn(id);
		}
	}
}

Object::Ptr DowntimesTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	return Service::GetOwnerByDowntimeID(row);
}

Value DowntimesTable::AuthorAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return downtime->Get("author");
}

Value DowntimesTable::CommentAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return downtime->Get("comment");
}

Value DowntimesTable::IdAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return downtime->Get("legacy_id");
}

Value DowntimesTable::EntryTimeAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return static_cast<int>(downtime->Get("entry_time"));
}

Value DowntimesTable::TypeAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);
	// 1 .. active, 0 .. pending
	return (Service::IsDowntimeActive(downtime) ? 1 : 0);
}

Value DowntimesTable::IsServiceAccessor(const Value& row)
{
	Service::Ptr svc = Service::GetOwnerByDowntimeID(row);

	return (svc->IsHostCheck() ? 0 : 1);
}

Value DowntimesTable::StartTimeAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return static_cast<int>(downtime->Get("start_time"));
}

Value DowntimesTable::EndTimeAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return static_cast<int>(downtime->Get("end_time"));
}

Value DowntimesTable::FixedAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return downtime->Get("fixed");
}

Value DowntimesTable::DurationAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return downtime->Get("duration");
}

Value DowntimesTable::TriggeredByAccessor(const Value& row)
{
	Dictionary::Ptr downtime = Service::GetDowntimeByID(row);

	return downtime->Get("triggered_by");
}
