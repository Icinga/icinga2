/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LOGTABLE_H
#define LOGTABLE_H

#include "livestatus/historytable.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class LogTable final : public HistoryTable
{
public:
	DECLARE_PTR_TYPEDEFS(LogTable);

	LogTable(const String& compat_log_path, time_t from, time_t until);

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

	void UpdateLogEntries(const Dictionary::Ptr& log_entry_attrs, int line_count, int lineno, const AddRowFunction& addRowFn) override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ContactAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr CommandAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value TimeAccessor(const Value& row);
	static Value LinenoAccessor(const Value& row);
	static Value ClassAccessor(const Value& row);
	static Value MessageAccessor(const Value& row);
	static Value TypeAccessor(const Value& row);
	static Value OptionsAccessor(const Value& row);
	static Value CommentAccessor(const Value& row);
	static Value PluginOutputAccessor(const Value& row);
	static Value StateAccessor(const Value& row);
	static Value StateTypeAccessor(const Value& row);
	static Value AttemptAccessor(const Value& row);
	static Value ServiceDescriptionAccessor(const Value& row);
	static Value HostNameAccessor(const Value& row);
	static Value ContactNameAccessor(const Value& row);
	static Value CommandNameAccessor(const Value& row);

private:
	std::map<time_t, String> m_LogFileIndex;
	std::map<time_t, Dictionary::Ptr> m_RowsCache;
	time_t m_TimeFrom;
	time_t m_TimeUntil;
	String m_CompatLogPath;
};

}

#endif /* LOGTABLE_H */
