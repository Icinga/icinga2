// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
