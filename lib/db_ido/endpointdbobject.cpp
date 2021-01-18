/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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

void EndpointDbObject::StaticInitialize()
{
	Endpoint::OnConnected.connect([](const Endpoint::Ptr& endpoint, const JsonRpcConnection::Ptr&) { EndpointDbObject::UpdateConnectedStatus(endpoint); });
	Endpoint::OnDisconnected.connect([](const Endpoint::Ptr& endpoint, const JsonRpcConnection::Ptr&) { EndpointDbObject::UpdateConnectedStatus(endpoint); });
}

EndpointDbObject::EndpointDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr EndpointDbObject::GetConfigFields() const
{
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(GetObject());

	return new Dictionary({
		{ "identity", endpoint->GetName() },
		{ "node", IcingaApplication::GetInstance()->GetNodeName() },
		{ "zone_object_id", endpoint->GetZone() }
	});
}

Dictionary::Ptr EndpointDbObject::GetStatusFields() const
{
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(GetObject());


	Log(LogDebug, "EndpointDbObject")
		<< "update status for endpoint '" << endpoint->GetName() << "'";

	return new Dictionary({
		{ "identity", endpoint->GetName() },
		{ "node", IcingaApplication::GetInstance()->GetNodeName() },
		{ "zone_object_id", endpoint->GetZone() },
		{ "is_connected", EndpointIsConnected(endpoint) }
	});
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

	query1.Fields = new Dictionary({
		{ "is_connected", (connected ? 1 : 0) },
		{ "status_update_time", DbValue::FromTimestamp(Utility::GetTime()) }
	});

	query1.WhereCriteria = new Dictionary({
		{ "endpoint_object_id", endpoint },
		{ "instance_id", 0 } /* DbConnection class fills in real ID */
	});

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
