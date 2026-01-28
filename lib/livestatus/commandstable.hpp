// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COMMANDSTABLE_H
#define COMMANDSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class CommandsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(CommandsTable);

	CommandsTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Value NameAccessor(const Value& row);
	static Value LineAccessor(const Value& row);
	static Value CustomVariableNamesAccessor(const Value& row);
	static Value CustomVariableValuesAccessor(const Value& row);
	static Value CustomVariablesAccessor(const Value& row);
};

}

#endif /* COMMANDSTABLE_H */
