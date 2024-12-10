/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "remote/i2-remote.hpp"
#include "remote/endpoint-ti.hpp"
#include "base/atomic.hpp"
#include "base/ringbuffer.hpp"
#include <cstdint>
#include <set>
#include <unordered_map>

namespace icinga
{

class ApiFunction;
class JsonRpcConnection;
class Zone;

/**
 * An endpoint that can be used to send and receive messages.
 *
 * @ingroup remote
 */
class Endpoint final : public ObjectImpl<Endpoint>
{
public:
	DECLARE_OBJECT(Endpoint);
	DECLARE_OBJECTNAME(Endpoint);

	static boost::signals2::signal<void(const Endpoint::Ptr&, const intrusive_ptr<JsonRpcConnection>&)> OnConnected;
	static boost::signals2::signal<void(const Endpoint::Ptr&, const intrusive_ptr<JsonRpcConnection>&)> OnDisconnected;

	Endpoint();

	void AddClient(const intrusive_ptr<JsonRpcConnection>& client);
	void RemoveClient(const intrusive_ptr<JsonRpcConnection>& client);
	std::set<intrusive_ptr<JsonRpcConnection> > GetClients() const;

	intrusive_ptr<Zone> GetZone() const;

	bool GetConnected() const override;

	static Endpoint::Ptr GetLocalEndpoint();

	void SetCachedZone(const intrusive_ptr<Zone>& zone);

	void AddMessageSent(int bytes);
	void AddMessageReceived(int bytes);
	void AddMessageReceived(const intrusive_ptr<ApiFunction>& method);

	void AddInputTimes(const AtomicDuration::Clock::duration& readTime, const AtomicDuration::Clock::duration& semaphoreTime, const AtomicDuration::Clock::duration& processTime)
	{
		m_InputReadTime += readTime;
		m_InputSemaphoreTime += semaphoreTime;
		m_InputProcessTime += processTime;
	}

	double GetMessagesSentPerSecond() const override;
	double GetMessagesReceivedPerSecond() const override;

	double GetBytesSentPerSecond() const override;
	double GetBytesReceivedPerSecond() const override;

	Dictionary::Ptr GetMessagesReceivedPerType() const override;

protected:
	void OnAllConfigLoaded() override;

private:
	mutable std::mutex m_ClientsLock;
	std::set<intrusive_ptr<JsonRpcConnection> > m_Clients;
	intrusive_ptr<Zone> m_Zone;
	std::unordered_map<intrusive_ptr<ApiFunction>, Atomic<uint_fast64_t>> m_MessageCounters;

	mutable RingBuffer m_MessagesSent{60};
	mutable RingBuffer m_MessagesReceived{60};
	mutable RingBuffer m_BytesSent{60};
	mutable RingBuffer m_BytesReceived{60};

	AtomicDuration m_InputReadTime;
	AtomicDuration m_InputSemaphoreTime;
	AtomicDuration m_InputProcessTime;
};

}

#endif /* ENDPOINT_H */
