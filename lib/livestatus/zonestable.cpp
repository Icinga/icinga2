/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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

	std::set<Endpoint::Ptr> endpoints = zone->GetEndpoints();

	Array::Ptr endpoint_names = new Array();

	for (const Endpoint::Ptr endpoint : endpoints) {
		endpoint_names->Add(endpoint->GetName());
	}

	if (!endpoint_names)
		return Empty;

	return endpoint_names;
}

Value ZonesTable::GlobalAccessor(const Value& row)
{
	Zone::Ptr zone = static_cast<Zone::Ptr>(row);

	if (!zone)
		return Empty;

	return zone->GetGlobal() ? 1 : 0;
}
