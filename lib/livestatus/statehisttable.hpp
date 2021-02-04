/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STATEHISTTABLE_H
#define STATEHISTTABLE_H

#include "icinga/service.hpp"
#include "livestatus/historytable.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class StateHistTable final : public HistoryTable
{
public:
	DECLARE_PTR_TYPEDEFS(StateHistTable);

	StateHistTable(const String& compat_log_path, time_t from, time_t until);

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

	void UpdateLogEntries(const Dictionary::Ptr& log_entry_attrs, int line_count, int lineno, const AddRowFunction& addRowFn) override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value TimeAccessor(const Value& row);
	static Value LinenoAccessor(const Value& row);
	static Value FromAccessor(const Value& row);
	static Value UntilAccessor(const Value& row);
	static Value DurationAccessor(const Value& row);
	static Value DurationPartAccessor(const Value& row);
	static Value StateAccessor(const Value& row);
	static Value HostDownAccessor(const Value& row);
	static Value InDowntimeAccessor(const Value& row);
	static Value InHostDowntimeAccessor(const Value& row);
	static Value IsFlappingAccessor(const Value& row);
	static Value InNotificationPeriodAccessor(const Value& row);
	static Value NotificationPeriodAccessor(const Value& row);
	static Value HostNameAccessor(const Value& row);
	static Value ServiceDescriptionAccessor(const Value& row);
	static Value LogOutputAccessor(const Value& row);
	static Value DurationOkAccessor(const Value& row);
	static Value DurationPartOkAccessor(const Value& row);
	static Value DurationWarningAccessor(const Value& row);
	static Value DurationPartWarningAccessor(const Value& row);
	static Value DurationCriticalAccessor(const Value& row);
	static Value DurationPartCriticalAccessor(const Value& row);
	static Value DurationUnknownAccessor(const Value& row);
	static Value DurationPartUnknownAccessor(const Value& row);
	static Value DurationUnmonitoredAccessor(const Value& row);
	static Value DurationPartUnmonitoredAccessor(const Value& row);

private:
	std::map<time_t, String> m_LogFileIndex;
	std::map<Checkable::Ptr, Array::Ptr> m_CheckablesCache;
	time_t m_TimeFrom;
	time_t m_TimeUntil;
	String m_CompatLogPath;
};

}

#endif /* STATEHISTTABLE_H */
