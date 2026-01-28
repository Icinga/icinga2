// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COMMENTSTABLE_H
#define COMMENTSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class CommentsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(CommentsTable);

	CommentsTable();

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
	static Value EntryTypeAccessor(const Value& row);
	static Value ExpiresAccessor(const Value& row);
	static Value ExpireTimeAccessor(const Value& row);
};

}

#endif /* COMMENTSTABLE_H */
