// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/zonestable.hpp"
#include "remote/zone.hpp"
#include "base/configtype.hpp"

using namespace icinga;

ZonesTable::ZonesTable()
{
	AddColumns(this);
}

void ZonesTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ZonesTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "parent", Column(&ZonesTable::ParentAccessor, objectAccessor));
	table->AddColumn(prefix + "endpoints", Column(&ZonesTable::EndpointsAccessor, objectAccessor));
	table->AddColumn(prefix + "global", Column(&ZonesTable::GlobalAccessor, objectAccessor));
}

String ZonesTable::GetName() const
{
	return "zones";
}

String ZonesTable::GetPrefix() const
{
	return "zone";
}

void ZonesTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
		if (!addRowFn(zone, LivestatusGroupByNone, Empty))
			return;
	}
}

Value ZonesTable::NameAccessor(const Value& row)
{
	Zone::Ptr zone = static_cast<Zone::Ptr>(row);

	if (!zone)
		return Empty;

	return zone->GetName();
}

Value ZonesTable::ParentAccessor(const Value& row)
{
	Zone::Ptr zone = static_cast<Zone::Ptr>(row);

	if (!zone)
		return Empty;

	Zone::Ptr parent_zone = zone->GetParent();

	if (!parent_zone)
		return Empty;

	return parent_zone->GetName();
}

Value ZonesTable::EndpointsAccessor(const Value& row)
{
	Zone::Ptr zone = static_cast<Zone::Ptr>(row);

	if (!zone)
		return Empty;

	auto endpoints (zone->GetEndpoints());
	ArrayData result;

	for (const Endpoint::Ptr& endpoint : endpoints) {
		result.push_back(endpoint->GetName());
	}

	return new Array(std::move(result));
}

Value ZonesTable::GlobalAccessor(const Value& row)
{
	Zone::Ptr zone = static_cast<Zone::Ptr>(row);

	if (!zone)
		return Empty;

	return zone->GetGlobal() ? 1 : 0;
}
