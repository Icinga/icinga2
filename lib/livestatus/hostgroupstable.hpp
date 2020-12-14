/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class HostGroupsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(HostGroupsTable);

	HostGroupsTable();

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
	static Value WorstHostStateAccessor(const Value& row);
	static Value NumHostsAccessor(const Value& row);
	static Value NumHostsPendingAccessor(const Value& row);
	static Value NumHostsUpAccessor(const Value& row);
	static Value NumHostsDownAccessor(const Value& row);
	static Value NumHostsUnreachAccessor(const Value& row);
	static Value NumServicesAccessor(const Value& row);
	static Value WorstServiceStateAccessor(const Value& row);
	static Value NumServicesPendingAccessor(const Value& row);
	static Value NumServicesOkAccessor(const Value& row);
	static Value NumServicesWarnAccessor(const Value& row);
	static Value NumServicesCritAccessor(const Value& row);
	static Value NumServicesUnknownAccessor(const Value& row);
	static Value WorstServiceHardStateAccessor(const Value& row);
	static Value NumServicesHardOkAccessor(const Value& row);
	static Value NumServicesHardWarnAccessor(const Value& row);
	static Value NumServicesHardCritAccessor(const Value& row);
	static Value NumServicesHardUnknownAccessor(const Value& row);
};

}
