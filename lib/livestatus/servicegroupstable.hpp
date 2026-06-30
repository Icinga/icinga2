// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SERVICEGROUPSTABLE_H
#define SERVICEGROUPSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class ServiceGroupsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(ServiceGroupsTable);

	ServiceGroupsTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Value NameAccessor(const Value& row);
	static Value AliasAccessor(const Value& row);
	static Value NotesAccessor(const Value& row);
	static Value NotesUrlAccessor(const Value& row);
	static Value ActionUrlAccessor(const Value& row);
	static Value MembersAccessor(const Value& row);
	static Value MembersWithStateAccessor(const Value& row);
	static Value WorstServiceStateAccessor(const Value& row);
	static Value NumServicesAccessor(const Value& row);
	static Value NumServicesOkAccessor(const Value& row);
	static Value NumServicesWarnAccessor(const Value& row);
	static Value NumServicesCritAccessor(const Value& row);
	static Value NumServicesUnknownAccessor(const Value& row);
	static Value NumServicesPendingAccessor(const Value& row);
	static Value NumServicesHardOkAccessor(const Value& row);
	static Value NumServicesHardWarnAccessor(const Value& row);
	static Value NumServicesHardCritAccessor(const Value& row);
	static Value NumServicesHardUnknownAccessor(const Value& row);
};

}

#endif /* SERVICEGROUPSTABLE_H */
