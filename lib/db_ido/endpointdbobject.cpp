/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "db_ido/endpointdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_DBTYPE(Endpoint, "endpoint", DbObjectTypeEndpoint, "endpoint_object_id", EndpointDbObject);

INITIALIZE_ONCE(&EndpointDbObject::StaticInitialize);

void EndpointDbObject::StaticInitialize(void)
{
	Endpoint::OnConnected.connect(std::bind(&EndpointDbObject::UpdateConnectedStatus, _1));
	Endpoint::OnDisconnected.connect(std::bind(&EndpointDbObject::UpdateConnectedStatus, _1));
}

EndpointDbObject::EndpointDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr EndpointDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(GetObject());

	fields->Set("identity", endpoint->GetName());
	fields->Set("node", IcingaApplication::GetInstance()->GetNodeName());
	fields->Set("zone_object_id", endpoint->GetZone());

	return fields;
}

Dictionary::Ptr EndpointDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = new Dictionary();
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(GetObject());

	Log(LogDebug, "EndpointDbObject")
	    << "update status for endpoint '" << endpoint->GetName() << "'";

	fields->Set("identity", endpoint->GetName());
	fields->Set("node", IcingaApplication::GetInstance()->GetNodeName());
	fields->Set("zone_object_id", endpoint->GetZone());
	fields->Set("is_connected", EndpointIsConnected(endpoint));

	return fields;
}

void EndpointDbObject::UpdateConnectedStatus(const Endpoint::Ptr& endpoint)
{
	bool connected = EndpointIsConnected(endpoint);

	Log(LogDebug, "EndpointDbObject")
	    << "update is_connected=" << connected << " for endpoint '" << endpoint->GetName() << "'";

	DbQuery query1;
	query1.Table = "endpointstatus";
	query1.Type = DbQueryUpdate;
	query1.Category = DbCatState;

	Dictionary::Ptr fields1 = new Dictionary();
	fields1->Set("is_connected", (connected ? 1 : 0));
	fields1->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query1.Fields = fields1;

	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("endpoint_object_id", endpoint);
	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query1);
}

int EndpointDbObject::EndpointIsConnected(const Endpoint::Ptr& endpoint)
{
	unsigned int is_connected = endpoint->GetConnected() ? 1 : 0;

	/* if identity is equal to node, fake is_connected */
	if (endpoint->GetName() == IcingaApplication::GetInstance()->GetNodeName())
		is_connected = 1;

	return is_connected;
}
