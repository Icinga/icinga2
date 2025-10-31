/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/redisconnection.hpp"
#include "base/convert.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/logger.hpp"
#include "base/string.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsutility.hpp"
#include "base/utility.hpp"
#include <boost/asio.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <exception>
#include <future>
#include <iterator>
#include <memory>
#include <openssl/ssl.h>
#include <utility>

using namespace icinga;
namespace asio = boost::asio;

boost::regex RedisConnection::m_ErrAuth ("\\AERR AUTH ");

RedisConnection::RedisConnection(const RedisConnInfo::ConstPtr& connInfo, const Ptr& parent, bool trackOwnPendingQueries)
	: RedisConnection{IoEngine::Get().GetIoContext(), connInfo, parent, trackOwnPendingQueries}
{
}

RedisConnection::RedisConnection(boost::asio::io_context& io, const RedisConnInfo::ConstPtr& connInfo, const Ptr& parent, bool trackOwnPendingQueries)
	: m_ConnInfo{connInfo}, m_Strand(io), m_Connecting(false), m_Connected(false), m_Stopped(false),
	m_QueuedWrites(io), m_QueuedReads(io), m_TrackOwnPendingQueries{trackOwnPendingQueries}, m_LogStatsTimer(io),
	m_Parent(parent)
{
	if (connInfo->EnableTls && connInfo->Path.IsEmpty()) {
		UpdateTLSContext();
	}
}

void RedisConnection::UpdateTLSContext()
{
	m_TLSContext = SetupSslContext(
		m_ConnInfo->TlsCertPath,
		m_ConnInfo->TlsKeyPath,
		m_ConnInfo->TlsCaPath,
		m_ConnInfo->TlsCrlPath,
		m_ConnInfo->TlsCipherList,
		m_ConnInfo->TlsProtocolMin,
		m_ConnInfo->DbgInfo
	);
}

void RedisConnection::Start()
{
	ASSERT(!m_Connected && !m_Connecting);

	Ptr keepAlive (this);
	m_Stopped.store(false);

	IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { ReadLoop(yc); });
	IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { WriteLoop(yc); });

	if (!m_Parent) {
		IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { LogStats(yc); });
	}

	m_Connecting.store(true);
	IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { Connect(yc); });
}

/**
 * Disconnect the Redis connection gracefully.
 *
 * This function initiates a graceful disconnection of the Redis connection. It sets the stopped flag to true,
 * and spawns a coroutine to handle the disconnection process. The coroutine waits for any ongoing read and write
 * operations to complete before shutting down the connection.
 */
void RedisConnection::Disconnect()
{
	if (!m_Stopped.exchange(true)) {
		IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive = Ptr(this)](asio::yield_context yc) {
			m_QueuedWrites.Set(); // Wake up write loop
			m_QueuedReads.Set(); // Wake up read loop

			// Give the read and write loops some time to finish ongoing operations before disconnecting.
			asio::deadline_timer waiter(m_Strand.context(), boost::posix_time::seconds(5));
			waiter.async_wait(yc);

			Log(m_Parent ? LogNotice : LogInformation, "IcingaDB")
				<< "Disconnecting Redis connection.";

			boost::system::error_code ec;
			if (m_TlsConn) {
				m_TlsConn->GracefulDisconnect(m_Strand, yc);
				m_TlsConn = nullptr;
			} else if (m_TcpConn) {
				m_TcpConn->lowest_layer().shutdown(Tcp::socket::shutdown_both, ec);
				m_TcpConn->lowest_layer().close(ec);
				m_TcpConn = nullptr;
			} else if (m_UnixConn) {
				m_UnixConn->lowest_layer().shutdown(Unix::socket::shutdown_both, ec);
				m_UnixConn->lowest_layer().close(ec);
				m_UnixConn = nullptr;
			}

			m_Connected.store(false);
			m_Connecting.store(false);
		});
	}
}

/**
 * Append a Redis query to a log message
 *
 * @param query Redis query
 * @param msg Log message
 */
static inline
void LogQuery(RedisConnection::Query& query, Log& msg)
{
	int i = 0;

	for (auto& arg : query) {
		if (++i == 8) {
			msg << " ...";
			break;
		}

		if (arg.GetLength() > 64) {
			msg << " '" << arg.SubStr(0, 61) << "...'";
		} else {
			msg << " '" << arg << '\'';
		}
	}
}

/**
 * Queue a Redis query for sending without waiting for the response in a fire-and-forget manner.
 *
 * If the highPriority flag is set to true, the query is treated with high priority and placed at the front of
 * the write queue, ensuring it is sent before other queued queries. This is useful for time-sensitive operations
 * that require to be executed promptly, which is the case for IcingaDB heartbeat queries. If there are already
 * queries with high priority in the queue, the new query is inserted after all existing high priority queries but
 * before any normal priority queries to maintain the order of high priority items.
 *
 * @note The highPriority flag should be used sparingly and only for critical queries, as it can affect the overall
 * performance and responsiveness of the Redis connection by potentially delaying other queued queries.
 *
 * @param query The Redis query to be sent.
 * @param affects Does the query affect config, state or history data.
 * @param highPriority Whether the query should be treated with high priority.
 */
void RedisConnection::FireAndForgetQuery(Query query, QueryAffects affects, bool highPriority)
{
	ThrowIfStopped();

	if (LogDebug >= Logger::GetMinLogSeverity()) {
		Log msg (LogDebug, "IcingaDB", "Firing and forgetting query:");
		LogQuery(query, msg);
	}

	auto item (Shared<Query>::Make(std::move(query)));

	asio::post(m_Strand, [this, item, highPriority, affects, ctime = Utility::GetTime()]() {
		m_Queues.Push(WriteQueueItem{item, ctime, affects}, highPriority);
		m_QueuedWrites.Set();
		IncreasePendingQueries(1);
	});
}

/**
 * Queue Redis queries for sending
 *
 * @param queries Redis queries
 */
void RedisConnection::FireAndForgetQueries(RedisConnection::Queries queries, QueryAffects affects)
{
	ThrowIfStopped();

	if (LogDebug >= Logger::GetMinLogSeverity()) {
		for (auto& query : queries) {
			Log msg(LogDebug, "IcingaDB", "Firing and forgetting query:");
			LogQuery(query, msg);
		}
	}

	auto item (Shared<Queries>::Make(std::move(queries)));

	asio::post(m_Strand, [this, item, affects, ctime = Utility::GetTime()]() {
		m_Queues.Push(WriteQueueItem{item, ctime, affects}, false);
		m_QueuedWrites.Set();
		IncreasePendingQueries(item->size());
	});
}

/**
 * Queue a Redis query for sending, wait for the response and return (or throw) it
 *
 * @param query Redis query
 *
 * @return The response
 */
RedisConnection::Reply RedisConnection::GetResultOfQuery(RedisConnection::Query query, QueryAffects affects)
{
	ThrowIfStopped();

	if (LogDebug >= Logger::GetMinLogSeverity()) {
		Log msg (LogDebug, "IcingaDB", "Executing query:");
		LogQuery(query, msg);
	}

	std::promise<Reply> promise;
	auto future (promise.get_future());
	auto item (Shared<std::pair<Query, std::promise<Reply>>>::Make(std::move(query), std::move(promise)));

	asio::post(m_Strand, [this, item, affects, ctime = Utility::GetTime()]() {
		m_Queues.Push(WriteQueueItem{item, ctime, affects}, false);
		m_QueuedWrites.Set();
		IncreasePendingQueries(1);
	});

	item = nullptr;
	future.wait();
	return future.get();
}

/**
 * Queue Redis queries for sending, wait for the responses and return (or throw) them
 *
 * @param queries Redis queries
 *
 * @return The responses
 */
RedisConnection::Replies RedisConnection::GetResultsOfQueries(Queries queries, QueryAffects affects, bool highPriority)
{
	ThrowIfStopped();

	if (LogDebug >= Logger::GetMinLogSeverity()) {
		for (auto& query : queries) {
			Log msg(LogDebug, "IcingaDB", "Executing query:");
			LogQuery(query, msg);
		}
	}

	std::promise<Replies> promise;
	auto future (promise.get_future());
	auto item (Shared<std::pair<Queries, std::promise<Replies>>>::Make(std::move(queries), std::move(promise)));

	asio::post(m_Strand, [this, item, highPriority, affects, ctime = Utility::GetTime()]() {
		m_Queues.Push(WriteQueueItem{item, ctime, affects}, highPriority);
		m_QueuedWrites.Set();
		IncreasePendingQueries(item->first.size());
	});

	item = nullptr;
	future.wait();
	return future.get();
}

void RedisConnection::EnqueueCallback(const std::function<void(boost::asio::yield_context&)>& callback)
{
	ThrowIfStopped();

	asio::post(m_Strand, [this, callback, ctime = Utility::GetTime()]() {
		m_Queues.Push(WriteQueueItem{callback, ctime}, false);
		m_QueuedWrites.Set();
	});
}

/**
 * Puts a no-op command with a result at the end of the queue and wait for the result,
 * i.e. for everything enqueued to be processed by the server.
 *
 * @ingroup icingadb
 */
void RedisConnection::Sync()
{
	GetResultOfQuery({"PING"});
}

/**
 * Get the enqueue time of the oldest still queued Redis query
 *
 * @return *nix timestamp or 0
 */
double RedisConnection::GetOldestPendingQueryTs() const
{
	auto promise (Shared<std::promise<double>>::Make());
	auto future (promise->get_future());

	asio::post(m_Strand, [this, promise]() {
		double oldest = 0;
		if (!m_Queues.HighWriteQ.empty()) {
			oldest = m_Queues.HighWriteQ.front().CTime;
		}
		if (!m_Queues.NormalWriteQ.empty()) {
			auto normalOldest = m_Queues.NormalWriteQ.front().CTime;
			if (oldest == 0 || normalOldest < oldest) {
				oldest = normalOldest;
			}
		}

		promise->set_value(oldest);
	});

	future.wait();
	return future.get();
}

/**
 * Try to connect to Redis
 */
void RedisConnection::Connect(asio::yield_context& yc)
{
	Defer notConnecting ([this]() { m_Connecting.store(m_Connected.load()); });

	boost::asio::deadline_timer timer (m_Strand.context());

	while (!m_Stopped) {
		try {
			if (m_ConnInfo->Path.IsEmpty()) {
				if (m_TLSContext) {
					Log(m_Parent ? LogNotice : LogInformation, "IcingaDB")
						<< "Trying to connect to Redis server (async, TLS) on host '" << m_ConnInfo->Host << ":" << m_ConnInfo->Port << "'";

					auto conn (Shared<AsioTlsStream>::Make(m_Strand.context(), *m_TLSContext, m_ConnInfo->Host));
					auto& tlsConn (conn->next_layer());
					auto connectTimeout (MakeTimeout(conn));

					icinga::Connect(conn->lowest_layer(), m_ConnInfo->Host, Convert::ToString(m_ConnInfo->Port), yc);
					tlsConn.async_handshake(tlsConn.client, yc);

					if (!m_ConnInfo->TlsInsecureNoverify) {
						std::shared_ptr<X509> cert (tlsConn.GetPeerCertificate());

						if (!cert) {
							BOOST_THROW_EXCEPTION(std::runtime_error(
								"Redis didn't present any TLS certificate."
							));
						}

						if (!tlsConn.IsVerifyOK()) {
							BOOST_THROW_EXCEPTION(std::runtime_error(
								"TLS certificate validation failed: " + std::string(tlsConn.GetVerifyError())
							));
						}
					}

					Handshake(conn, yc);
					m_QueuedReads.WaitForClear(yc);
					m_TlsConn = std::move(conn);
				} else {
					Log(m_Parent ? LogNotice : LogInformation, "IcingaDB")
						<< "Trying to connect to Redis server (async) on host '" << m_ConnInfo->Host << ":" << m_ConnInfo->Port << "'";

					auto conn (Shared<TcpConn>::Make(m_Strand.context()));
					auto connectTimeout (MakeTimeout(conn));

					icinga::Connect(conn->next_layer(), m_ConnInfo->Host, Convert::ToString(m_ConnInfo->Port), yc);
					Handshake(conn, yc);
					m_QueuedReads.WaitForClear(yc);
					m_TcpConn = std::move(conn);
				}
			} else {
				Log(LogInformation, "IcingaDB")
					<< "Trying to connect to Redis server (async) on unix socket path '" << m_ConnInfo->Path << "'";

				auto conn (Shared<UnixConn>::Make(m_Strand.context()));
				auto connectTimeout (MakeTimeout(conn));

				conn->next_layer().async_connect(Unix::endpoint(m_ConnInfo->Path.CStr()), yc);
				Handshake(conn, yc);
				m_QueuedReads.WaitForClear(yc);
				m_UnixConn = std::move(conn);
			}

			m_Connected.store(true);

			Log(m_Parent ? LogNotice : LogInformation, "IcingaDB", "Connected to Redis server");

			// Operate on a copy so that the callback can set a new callback without destroying itself while running.
			auto callback (m_ConnectedCallback);
			if (callback) {
				callback(yc);
			}

			break;
		} catch (const std::exception& ex) {
			Log(LogCritical, "IcingaDB")
				<< "Cannot connect to Redis server ('"
				<< (m_ConnInfo->Path.IsEmpty() ? m_ConnInfo->Host+":"+Convert::ToString(m_ConnInfo->Port) : m_ConnInfo->Path)
				<< "'): " << ex.what();
		}

		timer.expires_from_now(boost::posix_time::seconds(5));
		timer.async_wait(yc);
	}

}

/**
 * Actually receive the responses to the Redis queries send by WriteItem() and handle them
 */
void RedisConnection::ReadLoop(asio::yield_context& yc)
{
	while (!m_Stopped) {
		m_QueuedReads.WaitForSet(yc);

		while (!m_Queues.FutureResponseActions.empty()) {
			auto item (std::move(m_Queues.FutureResponseActions.front()));
			m_Queues.FutureResponseActions.pop();

			switch (item.Action) {
				case ResponseAction::Ignore:
					try {
						for (auto i (item.Amount); i; --i) {
							ReadOne(yc);
						}
					} catch (const std::exception& ex) {
						Log(LogCritical, "IcingaDB")
							<< "Error during receiving the response to a query which has been fired and forgotten: " << ex.what();

						continue;
					}

					break;
				case ResponseAction::Deliver:
					for (auto i (item.Amount); i; --i) {
						auto promise (std::move(m_Queues.ReplyPromises.front()));
						m_Queues.ReplyPromises.pop();

						Reply reply;

						try {
							reply = ReadOne(yc);
						} catch (const std::exception&) {
							promise.set_exception(std::current_exception());

							continue;
						}

						promise.set_value(std::move(reply));
					}

					break;
				case ResponseAction::DeliverBulk:
					{
						auto promise (std::move(m_Queues.RepliesPromises.front()));
						m_Queues.RepliesPromises.pop();

						Replies replies;
						replies.reserve(item.Amount);

						for (auto i (item.Amount); i; --i) {
							try {
								replies.emplace_back(ReadOne(yc));
							} catch (const std::exception&) {
								promise.set_exception(std::current_exception());
								break;
							}
						}

						try {
							promise.set_value(std::move(replies));
						} catch (const std::future_error&) {
							// Complaint about the above op is not allowed
							// due to promise.set_exception() was already called
						}
					}
			}
		}

		m_QueuedReads.Clear();
	}
}

/**
 * Actually send the Redis queries queued by {FireAndForget,GetResultsOf}{Query,Queries}()
 */
void RedisConnection::WriteLoop(asio::yield_context& yc)
{
	while (!m_Stopped) {
		m_QueuedWrites.Wait(yc);

		while (m_Queues.HasWrites()) {
			auto queuedWrite(m_Queues.PopFront());
			std::visit(
				[this, &yc, &queuedWrite](const auto& item) {
					if (WriteItem(item, yc)) {
						RecordAffected(queuedWrite.Affects, Utility::GetTime());
					}
				},
				std::move(queuedWrite.Item)
			);
		}

		m_QueuedWrites.Clear();
	}
}

/**
 * Periodically log current query performance
 */
void RedisConnection::LogStats(asio::yield_context& yc)
{
	double lastMessage = 0;

	m_LogStatsTimer.expires_from_now(boost::posix_time::seconds(10));

	while (!m_Stopped) {
		m_LogStatsTimer.async_wait(yc);
		m_LogStatsTimer.expires_from_now(boost::posix_time::seconds(10));

		if (!IsConnected())
			continue;

		auto now (Utility::GetTime());
		bool timeoutReached = now - lastMessage >= 5 * 60;

		if (m_PendingQueries < 1 && !timeoutReached)
			continue;

		auto output (round(m_OutputQueries.CalculateRate(now, 10)));

		if (m_PendingQueries < output * 5 && !timeoutReached)
			continue;

		Log(LogInformation, "IcingaDB")
			<< "Pending queries: " << m_PendingQueries << " (Input: "
			<< round(m_InputQueries.CalculateRate(now, 10)) << "/s; Output: " << output << "/s)";

		lastMessage = now;
	}
}

/**
 * Write a single Redis query in a fire-and-forget manner.
 *
 * @param item Redis query
 *
 * @return true on success, false on failure.
 */
bool RedisConnection::WriteItem(const FireAndForgetQ& item, boost::asio::yield_context& yc)
{
	DecreasePendingQueries(1);

	try {
		WriteOne(*item, yc);
	} catch (const std::exception& ex) {
		Log msg (LogCritical, "IcingaDB", "Error during sending query");
		LogQuery(*item, msg);
		msg << " which has been fired and forgotten: " << ex.what();

		return false;
	}

	if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Ignore) {
		m_Queues.FutureResponseActions.emplace(FutureResponseAction{1, ResponseAction::Ignore});
	} else {
		++m_Queues.FutureResponseActions.back().Amount;
	}

	m_QueuedReads.Set();
	return true;
}

/**
 * Write multiple Redis queries in a fire-and-forget manner.
 *
 * @param item Redis queries
 *
 * @return true on success, false on failure.
 */
bool RedisConnection::WriteItem(const FireAndForgetQs& item, boost::asio::yield_context& yc)
{
	size_t i = 0;

	DecreasePendingQueries(item->size());

	try {
		for (auto& query : *item) {
			WriteOne(query, yc);
			++i;
		}
	} catch (const std::exception& ex) {
		Log msg (LogCritical, "IcingaDB", "Error during sending query");
		LogQuery((*item)[i], msg);
		msg << " which has been fired and forgotten: " << ex.what();

		return false;
	}

	if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Ignore) {
		m_Queues.FutureResponseActions.emplace(FutureResponseAction{item->size(), ResponseAction::Ignore});
	} else {
		m_Queues.FutureResponseActions.back().Amount += item->size();
	}

	m_QueuedReads.Set();
	return true;
}

/**
 * Write a single Redis query and enqueue a response promise to be fulfilled once the response has been received.
 *
 * @param item Redis query and promise for the response
 */
bool RedisConnection::WriteItem(const QueryWithPromise& item, boost::asio::yield_context& yc)
{
	DecreasePendingQueries(1);

	try {
		WriteOne(item->first, yc);
	} catch (const std::exception&) {
		item->second.set_exception(std::current_exception());

		return false;
	}

	m_Queues.ReplyPromises.push(std::move(item->second));

	if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Deliver) {
		m_Queues.FutureResponseActions.emplace(FutureResponseAction{1, ResponseAction::Deliver});
	} else {
		++m_Queues.FutureResponseActions.back().Amount;
	}

	m_QueuedReads.Set();
	return true;
}

/**
 * Write multiple Redis queries and enqueue a response promise to be fulfilled once all responses have been received.
 *
 * @param item Redis queries and promise for the responses.
 *
 * @return true on success, false on failure.
 */
bool RedisConnection::WriteItem(const QueriesWithPromise& item, boost::asio::yield_context& yc)
{
	DecreasePendingQueries(item->first.size());

	try {
		for (auto& query : item->first) {
			WriteOne(query, yc);
		}
	} catch (const std::exception&) {
		item->second.set_exception(std::current_exception());

		return false;
	}

	m_Queues.RepliesPromises.emplace(std::move(item->second));
	m_Queues.FutureResponseActions.emplace(FutureResponseAction{item->first.size(), ResponseAction::DeliverBulk});

	m_QueuedReads.Set();
	return true;
}

/**
 * Invokes the provided callback immediately.
 *
 * @param item Callback to execute
 */
bool RedisConnection::WriteItem(const QueryCallback& item, boost::asio::yield_context& yc)
{
	item(yc);
	return true;
}

/**
 * Receive the response to a Redis query
 *
 * @return The response
 */
RedisConnection::Reply RedisConnection::ReadOne(boost::asio::yield_context& yc)
{
	if (m_ConnInfo->Path.IsEmpty()) {
		if (m_TLSContext) {
			return ReadOne(m_TlsConn, yc);
		} else {
			return ReadOne(m_TcpConn, yc);
		}
	} else {
		return ReadOne(m_UnixConn, yc);
	}
}

/**
 * Send query
 *
 * @param query Redis query
 */
void RedisConnection::WriteOne(RedisConnection::Query& query, asio::yield_context& yc)
{
	if (m_ConnInfo->Path.IsEmpty()) {
		if (m_TLSContext) {
			WriteOne(m_TlsConn, query, yc);
		} else {
			WriteOne(m_TcpConn, query, yc);
		}
	} else {
		WriteOne(m_UnixConn, query, yc);
	}
}

/**
 * Specify a callback that is run each time a connection is successfully established
 *
 * The callback is executed from a Boost.Asio coroutine and should therefore not perform blocking operations.
 *
 * @param callback Callback to execute
 */
void RedisConnection::SetConnectedCallback(std::function<void(asio::yield_context& yc)> callback) {
	m_ConnectedCallback = std::move(callback);
}

int RedisConnection::GetQueryCount(RingBuffer::SizeType span)
{
	return m_OutputQueries.UpdateAndGetValues(Utility::GetTime(), span);
}

void RedisConnection::IncreasePendingQueries(int count)
{
	if (m_Parent) {
		auto parent (m_Parent);

		asio::post(parent->m_Strand, [parent, count]() {
			parent->IncreasePendingQueries(count);
		});
	}

	// Only track the pending queries of the root connection or if explicitly
	// requested to do so for child connections as well.
	if (!m_Parent || m_TrackOwnPendingQueries) {
		m_PendingQueries.fetch_add(count);
		m_InputQueries.InsertValue(Utility::GetTime(), count);
	}
}

void RedisConnection::DecreasePendingQueries(int count)
{
	if (m_Parent) {
		auto parent (m_Parent);

		asio::post(parent->m_Strand, [parent, count]() {
			parent->DecreasePendingQueries(count);
		});
	}

	// Same as in IncreasePendingQueries().
	if (!m_Parent || m_TrackOwnPendingQueries) {
		m_PendingQueries.fetch_sub(count);
		m_OutputQueries.InsertValue(Utility::GetTime(), count);
	}
}

void RedisConnection::RecordAffected(RedisConnection::QueryAffects affected, double when)
{
	if (m_Parent) {
		auto parent (m_Parent);

		asio::post(parent->m_Strand, [parent, affected, when]() {
			parent->RecordAffected(affected, when);
		});
	} else {
		if (affected.Config) {
			m_WrittenConfig.InsertValue(when, affected.Config);
		}

		if (affected.State) {
			m_WrittenState.InsertValue(when, affected.State);
		}

		if (affected.History) {
			m_WrittenHistory.InsertValue(when, affected.History);
		}
	}
}

void RedisConnection::ThrowIfStopped() const
{
	if (m_Stopped) {
		throw RedisDisconnected();
	}
}
