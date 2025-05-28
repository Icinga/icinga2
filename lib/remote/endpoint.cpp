/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/endpoint.hpp"
#include "remote/endpoint-ti.cpp"
#include "remote/apilistener.hpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/zone.hpp"
#include "base/perfdatavalue.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

REGISTER_TYPE(Endpoint);

boost::signals2::signal<void(const Endpoint::Ptr&, const JsonRpcConnection::Ptr&)> Endpoint::OnConnected;
boost::signals2::signal<void(const Endpoint::Ptr&, const JsonRpcConnection::Ptr&)> Endpoint::OnDisconnected;

REGISTER_STATSFUNCTION(Endpoint, &Endpoint::StatsFunc);

void Endpoint::OnAllConfigLoaded()
{
	ObjectImpl<Endpoint>::OnAllConfigLoaded();

	if (!m_Zone)
		BOOST_THROW_EXCEPTION(ScriptError("Endpoint '" + GetName() +
			"' does not belong to a zone.", GetDebugInfo()));
}

void Endpoint::SetCachedZone(const Zone::Ptr& zone)
{
	if (m_Zone)
		BOOST_THROW_EXCEPTION(ScriptError("Endpoint '" + GetName()
			+ "' is in more than one zone.", GetDebugInfo()));

	m_Zone = zone;
}

void Endpoint::AddClient(const JsonRpcConnection::Ptr& client)
{
	bool was_master = ApiListener::GetInstance()->IsMaster();

	{
		std::unique_lock<std::mutex> lock(m_ClientsLock);
		m_Clients.insert(client);
	}

	bool is_master = ApiListener::GetInstance()->IsMaster();

	if (was_master != is_master)
		ApiListener::OnMasterChanged(is_master);

	OnConnected(this, client);
}

void Endpoint::RemoveClient(const JsonRpcConnection::Ptr& client)
{
	bool was_master = ApiListener::GetInstance()->IsMaster();

	{
		std::unique_lock<std::mutex> lock(m_ClientsLock);
		m_Clients.erase(client);

		Log(LogWarning, "ApiListener")
			<< "Removing API client for endpoint '" << GetName() << "'. " << m_Clients.size() << " API clients left.";

		SetConnecting(false);
	}

	bool is_master = ApiListener::GetInstance()->IsMaster();

	if (was_master != is_master)
		ApiListener::OnMasterChanged(is_master);

	OnDisconnected(this, client);
}

std::set<JsonRpcConnection::Ptr> Endpoint::GetClients() const
{
	std::unique_lock<std::mutex> lock(m_ClientsLock);
	return m_Clients;
}

Zone::Ptr Endpoint::GetZone() const
{
	return m_Zone;
}

bool Endpoint::GetConnected() const
{
	std::unique_lock<std::mutex> lock(m_ClientsLock);
	return !m_Clients.empty();
}

Endpoint::Ptr Endpoint::GetLocalEndpoint()
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return nullptr;

	return listener->GetLocalEndpoint();
}

void Endpoint::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	auto localZone (Zone::GetLocalZone());
	auto parentZone (localZone->GetParent());
	auto unorderedZones (ConfigType::GetObjectsByType<Zone>());
	std::set<Zone::Ptr> zones (unorderedZones.begin(), unorderedZones.end());
	std::set<Endpoint::Ptr> endpoints;
	Dictionary::Ptr ourStatus = new Dictionary;

	unorderedZones.clear();

	for (auto zone (zones.begin()); zone != zones.end();) {
		if ((*zone)->GetParent() == localZone) {
			++zone;
		} else {
			zones.erase(zone++);
		}
	}

	zones.emplace(localZone);

	if (parentZone)
		zones.emplace(parentZone);

	for (auto& zone : zones) {
		auto zoneEndpoints (zone->GetEndpoints());
		endpoints.insert(zoneEndpoints.begin(), zoneEndpoints.end());
	}

	endpoints.erase(GetLocalEndpoint());

	for (auto& endpoint : endpoints) {
		ourStatus->Set(endpoint->GetName(), new Dictionary({
			{"local_log_position", endpoint->GetLocalLogPosition()},
			{"remote_log_position", endpoint->GetRemoteLogPosition()},
			{"connecting", endpoint->GetConnecting()},
			{"syncing", endpoint->GetSyncing()},
			{"connected", endpoint->GetConnected()},
			{"last_message_sent", endpoint->GetLastMessageSent()},
			{"last_message_received", endpoint->GetLastMessageReceived()},
			{"messages_sent_per_second", endpoint->GetMessagesSentPerSecond()},
			{"messages_received_per_second", endpoint->GetMessagesReceivedPerSecond()},
			{"bytes_sent_per_second", endpoint->GetBytesSentPerSecond()},
			{"bytes_received_per_second", endpoint->GetBytesReceivedPerSecond()}
		}));
	}

	{
		ObjectLock ourStatusLock (ourStatus);

		for (auto& nameEndpointStatus : ourStatus) {
			Dictionary::Ptr endpointStatus = nameEndpointStatus.second;
			ObjectLock endpointStatusLock (endpointStatus);
			auto labelPrefix ("endpoint_" + nameEndpointStatus.first + "_");

			for (auto& labelValue : endpointStatus) {
				perfdata->Add(new PerfdataValue(labelPrefix + labelValue.first, labelValue.second));
			}
		}
	}

	status->Set("endpoint", ourStatus);
}

void Endpoint::AddMessageSent(int bytes)
{
	double time = Utility::GetTime();
	m_MessagesSent.InsertValue(time, 1);
	m_BytesSent.InsertValue(time, bytes);
	SetLastMessageSent(time);
}

void Endpoint::AddMessageReceived(int bytes)
{
	double time = Utility::GetTime();
	m_MessagesReceived.InsertValue(time, 1);
	m_BytesReceived.InsertValue(time, bytes);
	SetLastMessageReceived(time);
}

double Endpoint::GetMessagesSentPerSecond() const
{
	return m_MessagesSent.CalculateRate(Utility::GetTime(), 60);
}

double Endpoint::GetMessagesReceivedPerSecond() const
{
	return m_MessagesReceived.CalculateRate(Utility::GetTime(), 60);
}

double Endpoint::GetBytesSentPerSecond() const
{
	return m_BytesSent.CalculateRate(Utility::GetTime(), 60);
}

double Endpoint::GetBytesReceivedPerSecond() const
{
	return m_BytesReceived.CalculateRate(Utility::GetTime(), 60);
}
