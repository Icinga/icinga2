// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOWNTIMESTABLE_H
#define DOWNTIMESTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class DowntimesTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(DowntimesTable);

	DowntimesTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

private:
	static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value AuthorAccessor(const Value& row);
	static Value CommentAccessor(const Value& row);
	static Value IdAccessor(const Value& row);
	static Value EntryTimeAccessor(const Value& row);
	static Value TypeAccessor(const Value& row);
	static Value IsServiceAccessor(const Value& row);
	static Value StartTimeAccessor(const Value& row);
	static Value EndTimeAccessor(const Value& row);
	static Value FixedAccessor(const Value& row);
	static Value DurationAccessor(const Value& row);
	static Value TriggeredByAccessor(const Value& row);
};

}

#endif /* DOWNTIMESTABLE_H */
