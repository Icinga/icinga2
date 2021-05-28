/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/timeperioddbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/legacytimeperiod.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

REGISTER_DBTYPE(TimePeriod, "timeperiod", DbObjectTypeTimePeriod, "timeperiod_object_id", TimePeriodDbObject);

TimePeriodDbObject::TimePeriodDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr TimePeriodDbObject::GetConfigFields() const
{
	TimePeriod::Ptr tp = static_pointer_cast<TimePeriod>(GetObject());

	return new Dictionary({
		{ "alias", tp->GetDisplayName() }
	});
}

Dictionary::Ptr TimePeriodDbObject::GetStatusFields() const
{
	return Empty;
}

void TimePeriodDbObject::OnConfigUpdateHeavy(std::vector<DbQuery>&)
{
	TimePeriod::Ptr tp = static_pointer_cast<TimePeriod>(GetObject());

	DbQuery query_del1;
	query_del1.Table = GetType()->GetTable() + "_timeranges";
	query_del1.Type = DbQueryDelete;
	query_del1.Category = DbCatConfig;
	query_del1.WhereCriteria = new Dictionary({
		{ "timeperiod_id", DbValue::FromObjectInsertID(tp) }
	});
	OnQuery(query_del1);

	Dictionary::Ptr ranges = tp->GetRanges();

	if (!ranges)
		return;

	time_t refts = Utility::GetTime();
	ObjectLock olock(ranges);
	for (const Dictionary::Pair& kv : ranges) {
		int wday = LegacyTimePeriod::WeekdayFromString(kv.first);

		if (wday == -1)
			continue;

		tm reference = Utility::LocalTime(refts);

		Array::Ptr segments = new Array();
		LegacyTimePeriod::ProcessTimeRanges(kv.second, &reference, segments);

		ObjectLock olock(segments);
		for (const Value& vsegment : segments) {
			Dictionary::Ptr segment = vsegment;
			int begin = segment->Get("begin");
			int end = segment->Get("end");

			DbQuery query;
			query.Table = GetType()->GetTable() + "_timeranges";
			query.Type = DbQueryInsert;
			query.Category = DbCatConfig;
			query.Fields = new Dictionary({
				{ "instance_id", 0 }, /* DbConnection class fills in real ID */
				{ "timeperiod_id", DbValue::FromObjectInsertID(tp) },
				{ "day", wday },
				{ "start_sec", begin % 86400 },
				{ "end_sec", end % 86400 }
			});
			OnQuery(query);
		}
	}
}
