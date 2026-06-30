// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONTACTGROUPSTABLE_H
#define CONTACTGROUPSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class ContactGroupsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(ContactGroupsTable);

	ContactGroupsTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Value NameAccessor(const Value& row);
	static Value AliasAccessor(const Value& row);
	static Value MembersAccessor(const Value& row);
};

}

#endif /* CONTACTGROUPSTABLE_H */
