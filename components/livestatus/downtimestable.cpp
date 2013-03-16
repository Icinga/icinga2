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

#include "i2-livestatus.h"
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
	table->AddColumn(prefix + "author", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "comment", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "id", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_time", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "type", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "is_service", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "start_time", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "end_time", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "fixed", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "duration", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "triggered_by", Column(&Table::EmptyStringAccessor, objectAccessor));

	// TODO: Join services table.
}

String DowntimesTable::GetName(void) const
{
	return "downtimes";
}

void DowntimesTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);
		Dictionary::Ptr downtimes = service->GetDowntimes();

		if (!downtimes)
			continue;

		ObjectLock olock(downtimes);

		Value downtime;
		BOOST_FOREACH(boost::tie(boost::tuples::ignore, downtime), downtimes) {
			addRowFn(downtime);
		}
	}
}
