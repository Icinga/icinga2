// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ZONESTABLE_H
#define ZONESTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class ZonesTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(ZonesTable);

	ZonesTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Value NameAccessor(const Value& row);
	static Value ParentAccessor(const Value& row);
	static Value EndpointsAccessor(const Value& row);
	static Value GlobalAccessor(const Value& row);
};

}

#endif /* ZONESTABLE_H */
