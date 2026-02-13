// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/endpoint.hpp"
#include "remote/endpoint-ti.cpp"
#include "remote/apifunction.hpp"
#include "remote/apilistener.hpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/zone.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_TYPE(Endpoint);

boost::signals2::signal<void(const Endpoint::Ptr&, const JsonRpcConnection::Ptr&)> Endpoint::OnConnected;
boost::signals2::signal<void(const Endpoint::Ptr&, const JsonRpcConnection::Ptr&)> Endpoint::OnDisconnected;

INITIALIZE_ONCE(&Endpoint::ConfigStaticInitialize);

void Endpoint::ConfigStaticInitialize()
{
	OnLocalLogPositionChanged.connect([](const Endpoint::Ptr& ep, const Value&) {
		ep->GetReplayLog().Cleanup(ep->GetLocalLogPosition());
	});
}

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

Endpoint::Endpoint() : m_ReplayLog([this]() { return ReplayLog(GetName()); })
{
	for (auto& [name, afunc] : ApiFunctionRegistry::GetInstance()->GetItems()) {
		m_MessageCounters.emplace(afunc, 0);
	}
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

		Log(LogInformation, "ApiListener")
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

void Endpoint::AddMessageReceived(const intrusive_ptr<ApiFunction>& method)
{
	m_MessageCounters.at(method).fetch_add(1, std::memory_order_relaxed);
}

void Endpoint::AddMessageProcessed(const AtomicDuration::Clock::duration& duration)
{
	m_InputProcessingTime += duration;
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

Dictionary::Ptr Endpoint::GetMessagesReceivedPerType() const
{
	DictionaryData result;

	for (auto& [afunc, cnt] : m_MessageCounters) {
		if (auto v (cnt.load(std::memory_order_relaxed)); v) {
			result.emplace_back(afunc->GetName(), v);
		}
	}

	return new Dictionary(std::move(result));
}

double Endpoint::GetSecondsProcessingMessages() const
{
	return m_InputProcessingTime;
}
