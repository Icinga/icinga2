/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/redisconnection.hpp"
#include "base/array.hpp"
#include "base/convert.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/string.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsutility.hpp"
#include "base/utility.hpp"
#include <boost/asio.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/variant/get.hpp>
#include <exception>
#include <future>
#include <iterator>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <utility>

using namespace icinga;
namespace asio = boost::asio;

boost::regex RedisConnection::m_ErrAuth ("\\AERR AUTH ");

RedisConnection::RedisConnection(const String& host, int port, const String& path, const String& password, int db,
	bool useTls, bool insecure, const String& certPath, const String& keyPath, const String& caPath, const String& crlPath,
	const String& tlsProtocolmin, const String& cipherList, double connectTimeout, DebugInfo di, const RedisConnection::Ptr& parent)
	: RedisConnection(IoEngine::Get().GetIoContext(), host, port, path, password, db,
	  useTls, insecure, certPath, keyPath, caPath, crlPath, tlsProtocolmin, cipherList, connectTimeout, std::move(di), parent)
{
}

RedisConnection::RedisConnection(boost::asio::io_context& io, String host, int port, String path, String password,
	int db, bool useTls, bool insecure, String certPath, String keyPath, String caPath, String crlPath,
	String tlsProtocolmin, String cipherList, double connectTimeout, DebugInfo di, const RedisConnection::Ptr& parent)
	: m_Host(std::move(host)), m_Port(port), m_Path(std::move(path)), m_Password(std::move(password)),
	  m_DbIndex(db), m_CertPath(std::move(certPath)), m_KeyPath(std::move(keyPath)), m_Insecure(insecure),
	  m_CaPath(std::move(caPath)), m_CrlPath(std::move(crlPath)), m_TlsProtocolmin(std::move(tlsProtocolmin)),
	  m_CipherList(std::move(cipherList)), m_ConnectTimeout(connectTimeout), m_DebugInfo(std::move(di)), m_Connecting(false), m_Connected(false),
	  m_Started(false), m_Strand(io), m_QueuedWrites(io), m_QueuedReads(io), m_LogStatsTimer(io), m_Parent(parent)
{
	if (useTls && m_Path.IsEmpty()) {
		UpdateTLSContext();
	}
}

void RedisConnection::UpdateTLSContext()
{
	m_TLSContext = SetupSslContext(m_CertPath, m_KeyPath, m_CaPath,
		m_CrlPath, m_CipherList, m_TlsProtocolmin, m_DebugInfo);
}

void RedisConnection::Start()
{
	if (!m_Started.exchange(true)) {
		Ptr keepAlive (this);

		IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { ReadLoop(yc); });
		IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { WriteLoop(yc); });

		if (!m_Parent) {
			IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { LogStats(yc); });
		}
	}

	if (!m_Connecting.exchange(true)) {
		Ptr keepAlive (this);

		IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { Connect(yc); });
	}
}

bool RedisConnection::IsConnected() {
	return m_Connected.load();
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
 * Queue a Redis query for sending
 *
 * @param query Redis query
 * @param priority The query's priority
 */
void RedisConnection::FireAndForgetQuery(RedisConnection::Query query, RedisConnection::QueryPriority priority, QueryAffects affects)
{
	{
		Log msg (LogDebug, "IcingaDB", "Firing and forgetting query:");
		LogQuery(query, msg);
	}

	auto item (Shared<Query>::Make(std::move(query)));
	auto ctime (Utility::GetTime());

	asio::post(m_Strand, [this, item, priority, ctime, affects]() {
		m_Queues.Writes[priority].emplace(WriteQueueItem{item, nullptr, nullptr, nullptr, nullptr, ctime, affects});
		m_QueuedWrites.Set();
		IncreasePendingQueries(1);
	});
}

/**
 * Queue Redis queries for sending
 *
 * @param queries Redis queries
 * @param priority The queries' priority
 */
void RedisConnection::FireAndForgetQueries(RedisConnection::Queries queries, RedisConnection::QueryPriority priority, QueryAffects affects)
{
	for (auto& query : queries) {
		Log msg (LogDebug, "IcingaDB", "Firing and forgetting query:");
		LogQuery(query, msg);
	}

	auto item (Shared<Queries>::Make(std::move(queries)));
	auto ctime (Utility::GetTime());

	asio::post(m_Strand, [this, item, priority, ctime, affects]() {
		m_Queues.Writes[priority].emplace(WriteQueueItem{nullptr, item, nullptr, nullptr, nullptr, ctime, affects});
		m_QueuedWrites.Set();
		IncreasePendingQueries(item->size());
	});
}

/**
 * Queue a Redis query for sending, wait for the response and return (or throw) it
 *
 * @param query Redis query
 * @param priority The query's priority
 *
 * @return The response
 */
RedisConnection::Reply RedisConnection::GetResultOfQuery(RedisConnection::Query query, RedisConnection::QueryPriority priority, QueryAffects affects)
{
	{
		Log msg (LogDebug, "IcingaDB", "Executing query:");
		LogQuery(query, msg);
	}

	std::promise<Reply> promise;
	auto future (promise.get_future());
	auto item (Shared<std::pair<Query, std::promise<Reply>>>::Make(std::move(query), std::move(promise)));
	auto ctime (Utility::GetTime());

	asio::post(m_Strand, [this, item, priority, ctime, affects]() {
		m_Queues.Writes[priority].emplace(WriteQueueItem{nullptr, nullptr, item, nullptr, nullptr, ctime, affects});
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
 * @param priority The queries' priority
 *
 * @return The responses
 */
RedisConnection::Replies RedisConnection::GetResultsOfQueries(RedisConnection::Queries queries, RedisConnection::QueryPriority priority, QueryAffects affects)
{
	for (auto& query : queries) {
		Log msg (LogDebug, "IcingaDB", "Executing query:");
		LogQuery(query, msg);
	}

	std::promise<Replies> promise;
	auto future (promise.get_future());
	auto item (Shared<std::pair<Queries, std::promise<Replies>>>::Make(std::move(queries), std::move(promise)));
	auto ctime (Utility::GetTime());

	asio::post(m_Strand, [this, item, priority, ctime, affects]() {
		m_Queues.Writes[priority].emplace(WriteQueueItem{nullptr, nullptr, nullptr, item, nullptr, ctime, affects});
		m_QueuedWrites.Set();
		IncreasePendingQueries(item->first.size());
	});

	item = nullptr;
	future.wait();
	return future.get();
}

void RedisConnection::EnqueueCallback(const std::function<void(boost::asio::yield_context&)>& callback, RedisConnection::QueryPriority priority)
{
	auto ctime (Utility::GetTime());

	asio::post(m_Strand, [this, callback, priority, ctime]() {
		m_Queues.Writes[priority].emplace(WriteQueueItem{nullptr, nullptr, nullptr, nullptr, callback, ctime});
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
	GetResultOfQuery({"PING"}, RedisConnection::QueryPriority::SyncConnection);
}

/**
 * Get the enqueue time of the oldest still queued Redis query
 *
 * @return *nix timestamp or 0
 */
double RedisConnection::GetOldestPendingQueryTs()
{
	auto promise (Shared<std::promise<double>>::Make());
	auto future (promise->get_future());

	asio::post(m_Strand, [this, promise]() {
		double oldest = 0;

		for (auto& queue : m_Queues.Writes) {
			if (m_SuppressedQueryKinds.find(queue.first) == m_SuppressedQueryKinds.end() && !queue.second.empty()) {
				auto ctime (queue.second.front().CTime);

				if (ctime < oldest || oldest == 0) {
					oldest = ctime;
				}
			}
		}

		promise->set_value(oldest);
	});

	future.wait();
	return future.get();
}

/**
 * Mark kind as kind of queries not to actually send yet
 *
 * @param kind Query kind
 */
void RedisConnection::SuppressQueryKind(RedisConnection::QueryPriority kind)
{
	asio::post(m_Strand, [this, kind]() { m_SuppressedQueryKinds.emplace(kind); });
}

/**
 * Unmark kind as kind of queries not to actually send yet
 *
 * @param kind Query kind
 */
void RedisConnection::UnsuppressQueryKind(RedisConnection::QueryPriority kind)
{
	asio::post(m_Strand, [this, kind]() {
		m_SuppressedQueryKinds.erase(kind);
		m_QueuedWrites.Set();
	});
}

/**
 * Try to connect to Redis
 */
void RedisConnection::Connect(asio::yield_context& yc)
{
	Defer notConnecting ([this]() { m_Connecting.store(m_Connected.load()); });

	boost::asio::deadline_timer timer (m_Strand.context());

	auto waitForReadLoop ([this, &yc]() {
		while (!m_Queues.FutureResponseActions.empty()) {
			IoEngine::YieldCurrentCoroutine(yc);
		}
	});

	for (;;) {
		try {
			if (m_Path.IsEmpty()) {
				if (m_TLSContext) {
					Log(m_Parent ? LogNotice : LogInformation, "IcingaDB")
						<< "Trying to connect to Redis server (async, TLS) on host '" << m_Host << ":" << m_Port << "'";

					auto conn (Shared<AsioTlsStream>::Make(m_Strand.context(), *m_TLSContext, m_Host));
					auto& tlsConn (conn->next_layer());
					auto connectTimeout (MakeTimeout(conn));
					Defer cancelTimeout ([&connectTimeout]() { connectTimeout->Cancel(); });

					icinga::Connect(conn->lowest_layer(), m_Host, Convert::ToString(m_Port), yc);
					tlsConn.async_handshake(tlsConn.client, yc);

					if (!m_Insecure) {
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
					waitForReadLoop();
					m_TlsConn = std::move(conn);
				} else {
					Log(m_Parent ? LogNotice : LogInformation, "IcingaDB")
						<< "Trying to connect to Redis server (async) on host '" << m_Host << ":" << m_Port << "'";

					auto conn (Shared<TcpConn>::Make(m_Strand.context()));
					auto connectTimeout (MakeTimeout(conn));
					Defer cancelTimeout ([&connectTimeout]() { connectTimeout->Cancel(); });

					icinga::Connect(conn->next_layer(), m_Host, Convert::ToString(m_Port), yc);
					Handshake(conn, yc);
					waitForReadLoop();
					m_TcpConn = std::move(conn);
				}
			} else {
				Log(LogInformation, "IcingaDB")
					<< "Trying to connect to Redis server (async) on unix socket path '" << m_Path << "'";

				auto conn (Shared<UnixConn>::Make(m_Strand.context()));
				auto connectTimeout (MakeTimeout(conn));
				Defer cancelTimeout ([&connectTimeout]() { connectTimeout->Cancel(); });

				conn->next_layer().async_connect(Unix::endpoint(m_Path.CStr()), yc);
				Handshake(conn, yc);
				waitForReadLoop();
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
		} catch (const boost::coroutines::detail::forced_unwind&) {
			throw;
		} catch (const std::exception& ex) {
			Log(LogCritical, "IcingaDB")
				<< "Cannot connect to " << m_Host << ":" << m_Port << ": " << ex.what();
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
	for (;;) {
		m_QueuedReads.Wait(yc);

		while (!m_Queues.FutureResponseActions.empty()) {
			auto item (std::move(m_Queues.FutureResponseActions.front()));
			m_Queues.FutureResponseActions.pop();

			switch (item.Action) {
				case ResponseAction::Ignore:
					try {
						for (auto i (item.Amount); i; --i) {
							ReadOne(yc);
						}
					} catch (const boost::coroutines::detail::forced_unwind&) {
						throw;
					} catch (const std::exception& ex) {
						Log(LogCritical, "IcingaDB")
							<< "Error during receiving the response to a query which has been fired and forgotten: " << ex.what();

						continue;
					} catch (...) {
						Log(LogCritical, "IcingaDB")
							<< "Error during receiving the response to a query which has been fired and forgotten";

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
						} catch (const boost::coroutines::detail::forced_unwind&) {
							throw;
						} catch (...) {
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
							} catch (const boost::coroutines::detail::forced_unwind&) {
								throw;
							} catch (...) {
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
	for (;;) {
		m_QueuedWrites.Wait(yc);

	WriteFirstOfHighestPrio:
		for (auto& queue : m_Queues.Writes) {
			if (m_SuppressedQueryKinds.find(queue.first) != m_SuppressedQueryKinds.end() || queue.second.empty()) {
				continue;
			}

			auto next (std::move(queue.second.front()));
			queue.second.pop();

			WriteItem(yc, std::move(next));

			goto WriteFirstOfHighestPrio;
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

	for (;;) {
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
 * Send next and schedule receiving the response
 *
 * @param next Redis queries
 */
void RedisConnection::WriteItem(boost::asio::yield_context& yc, RedisConnection::WriteQueueItem next)
{
	if (next.FireAndForgetQuery) {
		auto& item (*next.FireAndForgetQuery);
		DecreasePendingQueries(1);

		try {
			WriteOne(item, yc);
		} catch (const boost::coroutines::detail::forced_unwind&) {
			throw;
		} catch (const std::exception& ex) {
			Log msg (LogCritical, "IcingaDB", "Error during sending query");
			LogQuery(item, msg);
			msg << " which has been fired and forgotten: " << ex.what();

			return;
		} catch (...) {
			Log msg (LogCritical, "IcingaDB", "Error during sending query");
			LogQuery(item, msg);
			msg << " which has been fired and forgotten";

			return;
		}

		if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Ignore) {
			m_Queues.FutureResponseActions.emplace(FutureResponseAction{1, ResponseAction::Ignore});
		} else {
			++m_Queues.FutureResponseActions.back().Amount;
		}

		m_QueuedReads.Set();
	}

	if (next.FireAndForgetQueries) {
		auto& item (*next.FireAndForgetQueries);
		size_t i = 0;

		DecreasePendingQueries(item.size());

		try {
			for (auto& query : item) {
				WriteOne(query, yc);
				++i;
			}
		} catch (const boost::coroutines::detail::forced_unwind&) {
			throw;
		} catch (const std::exception& ex) {
			Log msg (LogCritical, "IcingaDB", "Error during sending query");
			LogQuery(item[i], msg);
			msg << " which has been fired and forgotten: " << ex.what();

			return;
		} catch (...) {
			Log msg (LogCritical, "IcingaDB", "Error during sending query");
			LogQuery(item[i], msg);
			msg << " which has been fired and forgotten";

			return;
		}

		if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Ignore) {
			m_Queues.FutureResponseActions.emplace(FutureResponseAction{item.size(), ResponseAction::Ignore});
		} else {
			m_Queues.FutureResponseActions.back().Amount += item.size();
		}

		m_QueuedReads.Set();
	}

	if (next.GetResultOfQuery) {
		auto& item (*next.GetResultOfQuery);
		DecreasePendingQueries(1);

		try {
			WriteOne(item.first, yc);
		} catch (const boost::coroutines::detail::forced_unwind&) {
			throw;
		} catch (...) {
			item.second.set_exception(std::current_exception());

			return;
		}

		m_Queues.ReplyPromises.emplace(std::move(item.second));

		if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Deliver) {
			m_Queues.FutureResponseActions.emplace(FutureResponseAction{1, ResponseAction::Deliver});
		} else {
			++m_Queues.FutureResponseActions.back().Amount;
		}

		m_QueuedReads.Set();
	}

	if (next.GetResultsOfQueries) {
		auto& item (*next.GetResultsOfQueries);
		DecreasePendingQueries(item.first.size());

		try {
			for (auto& query : item.first) {
				WriteOne(query, yc);
			}
		} catch (const boost::coroutines::detail::forced_unwind&) {
			throw;
		} catch (...) {
			item.second.set_exception(std::current_exception());

			return;
		}

		m_Queues.RepliesPromises.emplace(std::move(item.second));
		m_Queues.FutureResponseActions.emplace(FutureResponseAction{item.first.size(), ResponseAction::DeliverBulk});

		m_QueuedReads.Set();
	}

	if (next.Callback) {
		next.Callback(yc);
	}

	RecordAffected(next.Affects, Utility::GetTime());
}

/**
 * Receive the response to a Redis query
 *
 * @return The response
 */
RedisConnection::Reply RedisConnection::ReadOne(boost::asio::yield_context& yc)
{
	if (m_Path.IsEmpty()) {
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
	if (m_Path.IsEmpty()) {
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
	} else {
		m_PendingQueries += count;
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
	} else {
		m_PendingQueries -= count;
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
