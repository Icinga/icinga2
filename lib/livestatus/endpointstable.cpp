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

#include "livestatus/endpointstable.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/icingaapplication.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace icinga;

EndpointsTable::EndpointsTable()
{
	AddColumns(this);
}

void EndpointsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&EndpointsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "identity", Column(&EndpointsTable::IdentityAccessor, objectAccessor));
	table->AddColumn(prefix + "node", Column(&EndpointsTable::NodeAccessor, objectAccessor));
	table->AddColumn(prefix + "is_connected", Column(&EndpointsTable::IsConnectedAccessor, objectAccessor));
	table->AddColumn(prefix + "zone", Column(&EndpointsTable::ZoneAccessor, objectAccessor));
}

String EndpointsTable::GetName() const
{
	return "endpoints";
}

String EndpointsTable::GetPrefix() const
{
	return "endpoint";
}

void EndpointsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>()) {
		if (!addRowFn(endpoint, LivestatusGroupByNone, Empty))
			return;
	}
}

Value EndpointsTable::NameAccessor(const Value& row)
{
	Endpoint::Ptr endpoint = static_cast<Endpoint::Ptr>(row);

	if (!endpoint)
		return Empty;

	return endpoint->GetName();
}

Value EndpointsTable::IdentityAccessor(const Value& row)
{
	Endpoint::Ptr endpoint = static_cast<Endpoint::Ptr>(row);

	if (!endpoint)
		return Empty;

	return endpoint->GetName();
}

Value EndpointsTable::NodeAccessor(const Value& row)
{
	Endpoint::Ptr endpoint = static_cast<Endpoint::Ptr>(row);

	if (!endpoint)
		return Empty;

	return IcingaApplication::GetInstance()->GetNodeName();
}

Value EndpointsTable::IsConnectedAccessor(const Value& row)
{
	Endpoint::Ptr endpoint = static_cast<Endpoint::Ptr>(row);

	if (!endpoint)
		return Empty;

	unsigned int is_connected = endpoint->GetConnected() ? 1 : 0;

	/* if identity is equal to node, fake is_connected */
	if (endpoint->GetName() == IcingaApplication::GetInstance()->GetNodeName())
		is_connected = 1;

	return is_connected;
}

Value EndpointsTable::ZoneAccessor(const Value& row)
{
	Endpoint::Ptr endpoint = static_cast<Endpoint::Ptr>(row);

	if (!endpoint)
		return Empty;

	Zone::Ptr zone = endpoint->GetZone();

	if (!zone)
		return Empty;

	return zone->GetName();
}
