// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include <boost/algorithm/string/replace.hpp>

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
