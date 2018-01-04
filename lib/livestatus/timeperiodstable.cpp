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

#include "livestatus/timeperiodstable.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/timeperiod.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

TimePeriodsTable::TimePeriodsTable()
{
	AddColumns(this);
}

void TimePeriodsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&TimePeriodsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&TimePeriodsTable::AliasAccessor, objectAccessor));
	table->AddColumn(prefix + "in", Column(&TimePeriodsTable::InAccessor, objectAccessor));
}

String TimePeriodsTable::GetName() const
{
	return "timeperiod";
}

String TimePeriodsTable::GetPrefix() const
{
	return "timeperiod";
}

void TimePeriodsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const TimePeriod::Ptr& tp : ConfigType::GetObjectsByType<TimePeriod>()) {
		if (!addRowFn(tp, LivestatusGroupByNone, Empty))
			return;
	}
}

Value TimePeriodsTable::NameAccessor(const Value& row)
{
	return static_cast<TimePeriod::Ptr>(row)->GetName();
}

Value TimePeriodsTable::AliasAccessor(const Value& row)
{
	return static_cast<TimePeriod::Ptr>(row)->GetDisplayName();
}

Value TimePeriodsTable::InAccessor(const Value& row)
{
	return (static_cast<TimePeriod::Ptr>(row)->IsInside(Utility::GetTime()) ? 1 : 0);
}
