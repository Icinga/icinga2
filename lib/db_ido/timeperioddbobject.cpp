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

#include "db_ido/timeperioddbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "icinga/timeperiod.h"
#include "icinga/legacytimeperiod.h"
#include "base/utility.h"
#include "base/exception.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

REGISTER_DBTYPE(TimePeriod, "timeperiod", DbObjectTypeTimePeriod, "timeperiod_object_id", TimePeriodDbObject);

TimePeriodDbObject::TimePeriodDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr TimePeriodDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	TimePeriod::Ptr tp = static_pointer_cast<TimePeriod>(GetObject());

	fields->Set("alias", tp->GetDisplayName());

	return fields;
}

Dictionary::Ptr TimePeriodDbObject::GetStatusFields(void) const
{
	return Empty;
}

void TimePeriodDbObject::OnConfigUpdate(void)
{
	TimePeriod::Ptr tp = static_pointer_cast<TimePeriod>(GetObject());

	DbQuery query_del1;
	query_del1.Table = GetType()->GetTable() + "_timeranges";
	query_del1.Type = DbQueryDelete;
	query_del1.WhereCriteria = boost::make_shared<Dictionary>();
	query_del1.WhereCriteria->Set("timeperiod_id", DbValue::FromObjectInsertID(tp));
	OnQuery(query_del1);

	Dictionary::Ptr ranges = tp->GetRanges();

	if (!ranges)
		return;

	time_t refts = Utility::GetTime();
	ObjectLock olock(ranges);
	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), ranges) {
		int wday = LegacyTimePeriod::WeekdayFromString(key);

		if (wday == -1)
			continue;

		tm reference;

#ifdef _MSC_VER
		tm *temp = localtime(&refts);

		if (temp == NULL) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("localtime")
			    << boost::errinfo_errno(errno));
		}

		reference = *temp;
#else /* _MSC_VER */
		if (localtime_r(&refts, &reference) == NULL) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("localtime_r")
			    << boost::errinfo_errno(errno));
		}
#endif /* _MSC_VER */

		Array::Ptr segments = boost::make_shared<Array>();
		LegacyTimePeriod::ProcessTimeRanges(value, &reference, segments);

		ObjectLock olock(segments);
		BOOST_FOREACH(const Value& vsegment, segments) {
			Dictionary::Ptr segment = vsegment;
			int begin = segment->Get("begin");
			int end = segment->Get("end");

			DbQuery query;
			query.Table = GetType()->GetTable() + "_timeranges";
			query.Type = DbQueryInsert;
			query.Fields = boost::make_shared<Dictionary>();
			query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query.Fields->Set("timeperiod_id", DbValue::FromObjectInsertID(tp));
			query.Fields->Set("day", wday);
			query.Fields->Set("start_sec", begin % 86400);
			query.Fields->Set("end_sec", end % 86400);
			OnQuery(query);
		}
	}
}
