/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef REDISCONNECTION_H
#define REDISCONNECTION_H

#include "base/array.hpp"
#include "base/atomic.hpp"
#include "base/convert.hpp"
#include "base/io-engine.hpp"
#include "base/object.hpp"
#include "base/ringbuffer.hpp"
#include "base/shared.hpp"
#include "base/string.hpp"
#include "base/tlsstream.hpp"
#include "base/value.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/utility/string_view.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <future>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace icinga
{
/**
 * An Async Redis connection.
 *
 * @ingroup icingadb
 */
	class RedisConnection final : public Object
	{
	public:
		DECLARE_PTR_TYPEDEFS(RedisConnection);

		typedef std::vector<String> Query;
		typedef std::vector<Query> Queries;
		typedef Value Reply;
		typedef std::vector<Reply> Replies;

		/**
		 * Redis query priorities, highest first.
		 *
		 * @ingroup icingadb
		 */
		enum class QueryPriority : unsigned char
		{
			Heartbeat,
			RuntimeStateStream, // runtime state updates, doesn't affect initially synced states
			Config, // includes initially synced states
			RuntimeStateSync, // updates initially synced states at runtime, in parallel to config dump, therefore must be < Config
			History,
			CheckResult,
			SyncConnection = 255
		};

		struct QueryAffects
		{
			size_t Config;
			size_t State;
			size_t History;

			QueryAffects(size_t config = 0, size_t state = 0, size_t history = 0)
				: Config(config), State(state), History(history) { }
		};

		RedisConnection(const String& host, int port, const String& path, const String& password, int db,
			bool useTls, bool insecure, const String& certPath, const String& keyPath, const String& caPath, const String& crlPath,
			const String& tlsProtocolmin, const String& cipherList, double connectTimeout, DebugInfo di, const Ptr& parent = nullptr);

		void UpdateTLSContext();

		void Start();

		bool IsConnected();

		void FireAndForgetQuery(Query query, QueryPriority priority, QueryAffects affects = {});
		void FireAndForgetQueries(Queries queries, QueryPriority priority, QueryAffects affects = {});

		Reply GetResultOfQuery(Query query, QueryPriority priority, QueryAffects affects = {});
		Replies GetResultsOfQueries(Queries queries, QueryPriority priority, QueryAffects affects = {});

		void EnqueueCallback(const std::function<void(boost::asio::yield_context&)>& callback, QueryPriority priority);
		void Sync();
		double GetOldestPendingQueryTs();

		void SuppressQueryKind(QueryPriority kind);
		void UnsuppressQueryKind(QueryPriority kind);

		void SetConnectedCallback(std::function<void(boost::asio::yield_context& yc)> callback);

		inline bool GetConnected()
		{
			return m_Connected.load();
		}

		int GetQueryCount(RingBuffer::SizeType span);

		inline int GetPendingQueryCount()
		{
			return m_PendingQueries;
		}

		inline int GetWrittenConfigFor(RingBuffer::SizeType span, RingBuffer::SizeType tv = Utility::GetTime())
		{
			return m_WrittenConfig.UpdateAndGetValues(tv, span);
		}

		inline int GetWrittenStateFor(RingBuffer::SizeType span, RingBuffer::SizeType tv = Utility::GetTime())
		{
			return m_WrittenState.UpdateAndGetValues(tv, span);
		}

		inline int GetWrittenHistoryFor(RingBuffer::SizeType span, RingBuffer::SizeType tv = Utility::GetTime())
		{
			return m_WrittenHistory.UpdateAndGetValues(tv, span);
		}

	private:
		/**
		 * What to do with the responses to Redis queries.
		 *
		 * @ingroup icingadb
		 */
		enum class ResponseAction : unsigned char
		{
			Ignore, // discard
			Deliver, // submit to the requestor
			DeliverBulk // submit multiple responses to the requestor at once
		};

		/**
		 * What to do with how many responses to Redis queries.
		 *
		 * @ingroup icingadb
		 */
		struct FutureResponseAction
		{
			size_t Amount;
			ResponseAction Action;
		};

		/**
		 * Something to be send to Redis.
		 *
		 * @ingroup icingadb
		 */
		struct WriteQueueItem
		{
			Shared<Query>::Ptr FireAndForgetQuery;
			Shared<Queries>::Ptr FireAndForgetQueries;
			Shared<std::pair<Query, std::promise<Reply>>>::Ptr GetResultOfQuery;
			Shared<std::pair<Queries, std::promise<Replies>>>::Ptr GetResultsOfQueries;
			std::function<void(boost::asio::yield_context&)> Callback;

			double CTime;
			QueryAffects Affects;
		};

		typedef boost::asio::ip::tcp Tcp;
		typedef boost::asio::local::stream_protocol Unix;

		typedef boost::asio::buffered_stream<Tcp::socket> TcpConn;
		typedef boost::asio::buffered_stream<Unix::socket> UnixConn;

		Shared<boost::asio::ssl::context>::Ptr m_TLSContext;

		template<class AsyncReadStream>
		static Value ReadRESP(AsyncReadStream& stream, boost::asio::yield_context& yc);

		template<class AsyncReadStream>
		static std::vector<char> ReadLine(AsyncReadStream& stream, boost::asio::yield_context& yc, size_t hint = 0);

		template<class AsyncWriteStream>
		static void WriteRESP(AsyncWriteStream& stream, const Query& query, boost::asio::yield_context& yc);

		static boost::regex m_ErrAuth;

		RedisConnection(boost::asio::io_context& io, String host, int port, String path, String password,
			int db, bool useTls, bool insecure, String certPath, String keyPath, String caPath, String crlPath,
			String tlsProtocolmin, String cipherList, double connectTimeout, DebugInfo di, const Ptr& parent);

		void Connect(boost::asio::yield_context& yc);
		void ReadLoop(boost::asio::yield_context& yc);
		void WriteLoop(boost::asio::yield_context& yc);
		void LogStats(boost::asio::yield_context& yc);
		void WriteItem(boost::asio::yield_context& yc, WriteQueueItem item);
		Reply ReadOne(boost::asio::yield_context& yc);
		void WriteOne(Query& query, boost::asio::yield_context& yc);

		template<class StreamPtr>
		Reply ReadOne(StreamPtr& stream, boost::asio::yield_context& yc);

		template<class StreamPtr>
		void WriteOne(StreamPtr& stream, Query& query, boost::asio::yield_context& yc);

		void IncreasePendingQueries(int count);
		void DecreasePendingQueries(int count);
		void RecordAffected(QueryAffects affected, double when);

		template<class StreamPtr>
		void Handshake(StreamPtr& stream, boost::asio::yield_context& yc);

		template<class StreamPtr>
		Timeout::Ptr MakeTimeout(StreamPtr& stream);

		String m_Path;
		String m_Host;
		int m_Port;
		String m_Password;
		int m_DbIndex;

		String m_CertPath;
		String m_KeyPath;
		bool m_Insecure;
		String m_CaPath;
		String m_CrlPath;
		String m_TlsProtocolmin;
		String m_CipherList;
		double m_ConnectTimeout;
		DebugInfo m_DebugInfo;

		boost::asio::io_context::strand m_Strand;
		Shared<TcpConn>::Ptr m_TcpConn;
		Shared<UnixConn>::Ptr m_UnixConn;
		Shared<AsioTlsStream>::Ptr m_TlsConn;
		Atomic<bool> m_Connecting, m_Connected, m_Started;

		struct {
			// Items to be send to Redis
			std::map<QueryPriority, std::queue<WriteQueueItem>> Writes;
			// Requestors, each waiting for a single response
			std::queue<std::promise<Reply>> ReplyPromises;
			// Requestors, each waiting for multiple responses at once
			std::queue<std::promise<Replies>> RepliesPromises;
			// Metadata about all of the above
			std::queue<FutureResponseAction> FutureResponseActions;
		} m_Queues;

		// Kinds of queries not to actually send yet
		std::set<QueryPriority> m_SuppressedQueryKinds;

		// Indicate that there's something to send/receive
		AsioConditionVariable m_QueuedWrites, m_QueuedReads;

		std::function<void(boost::asio::yield_context& yc)> m_ConnectedCallback;

		// Stats
		RingBuffer m_InputQueries{10};
		RingBuffer m_OutputQueries{15 * 60};
		RingBuffer m_WrittenConfig{15 * 60};
		RingBuffer m_WrittenState{15 * 60};
		RingBuffer m_WrittenHistory{15 * 60};
		int m_PendingQueries{0};
		boost::asio::deadline_timer m_LogStatsTimer;
		Ptr m_Parent;
	};

/**
 * An error response from the Redis server.
 *
 * @ingroup icingadb
 */
class RedisError final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(RedisError);

	inline RedisError(String message) : m_Message(std::move(message))
	{
	}

	inline const String& GetMessage()
	{
		return m_Message;
	}

private:
	String m_Message;
};

/**
 * Thrown if the connection to the Redis server has already been lost.
 *
 * @ingroup icingadb
 */
class RedisDisconnected : public std::runtime_error
{
public:
	inline RedisDisconnected() : runtime_error("")
	{
	}
};

/**
 * Thrown on malformed Redis server responses.
 *
 * @ingroup icingadb
 */
class RedisProtocolError : public std::runtime_error
{
protected:
	inline RedisProtocolError() : runtime_error("")
	{
	}
};

/**
 * Thrown on malformed types in Redis server responses.
 *
 * @ingroup icingadb
 */
class BadRedisType : public RedisProtocolError
{
public:
	inline BadRedisType(char type) : m_What{type, 0}
	{
	}

	virtual const char * what() const noexcept override
	{
		return m_What;
	}

private:
	char m_What[2];
};

/**
 * Thrown on malformed ints in Redis server responses.
 *
 * @ingroup icingadb
 */
class BadRedisInt : public RedisProtocolError
{
public:
	inline BadRedisInt(std::vector<char> intStr) : m_What(std::move(intStr))
	{
		m_What.emplace_back(0);
	}

	virtual const char * what() const noexcept override
	{
		return m_What.data();
	}

private:
	std::vector<char> m_What;
};

/**
 * Read a Redis server response from stream
 *
 * @param stream Redis server connection
 *
 * @return The response
 */
template<class StreamPtr>
RedisConnection::Reply RedisConnection::ReadOne(StreamPtr& stream, boost::asio::yield_context& yc)
{
	namespace asio = boost::asio;

	if (!stream) {
		throw RedisDisconnected();
	}

	auto strm (stream);

	try {
		return ReadRESP(*strm, yc);
	} catch (const boost::coroutines::detail::forced_unwind&) {
		throw;
	} catch (...) {
		if (m_Connecting.exchange(false)) {
			m_Connected.store(false);
			stream = nullptr;

			if (!m_Connecting.exchange(true)) {
				Ptr keepAlive (this);

				IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { Connect(yc); });
			}
		}

		throw;
	}
}

/**
 * Write a Redis query to stream
 *
 * @param stream Redis server connection
 * @param query Redis query
 */
template<class StreamPtr>
void RedisConnection::WriteOne(StreamPtr& stream, RedisConnection::Query& query, boost::asio::yield_context& yc)
{
	namespace asio = boost::asio;

	if (!stream) {
		throw RedisDisconnected();
	}

	auto strm (stream);

	try {
		WriteRESP(*strm, query, yc);
		strm->async_flush(yc);
	} catch (const boost::coroutines::detail::forced_unwind&) {
		throw;
	} catch (...) {
		if (m_Connecting.exchange(false)) {
			m_Connected.store(false);
			stream = nullptr;

			if (!m_Connecting.exchange(true)) {
				Ptr keepAlive (this);

				IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](asio::yield_context yc) { Connect(yc); });
			}
		}

		throw;
	}
}

/**
 * Initialize a Redis stream
 *
 * @param stream Redis server connection
 * @param query Redis query
 */
template<class StreamPtr>
void RedisConnection::Handshake(StreamPtr& strm, boost::asio::yield_context& yc)
{
	if (m_Password.IsEmpty() && !m_DbIndex) {
		// Trigger NOAUTH
		WriteRESP(*strm, {"PING"}, yc);
	} else {
		if (!m_Password.IsEmpty()) {
			WriteRESP(*strm, {"AUTH", m_Password}, yc);
		}

		if (m_DbIndex) {
			WriteRESP(*strm, {"SELECT", Convert::ToString(m_DbIndex)}, yc);
		}
	}

	strm->async_flush(yc);

	if (m_Password.IsEmpty() && !m_DbIndex) {
		Reply pong (ReadRESP(*strm, yc));

		if (pong.IsObjectType<RedisError>()) {
			// Likely NOAUTH
			BOOST_THROW_EXCEPTION(std::runtime_error(RedisError::Ptr(pong)->GetMessage()));
		}
	} else {
		if (!m_Password.IsEmpty()) {
			Reply auth (ReadRESP(*strm, yc));

			if (auth.IsObjectType<RedisError>()) {
				auto& authErr (RedisError::Ptr(auth)->GetMessage().GetData());
				boost::smatch what;

				if (boost::regex_search(authErr, what, m_ErrAuth)) {
					Log(LogWarning, "IcingaDB") << authErr;
				} else {
					// Likely WRONGPASS
					BOOST_THROW_EXCEPTION(std::runtime_error(authErr));
				}
			}
		}

		if (m_DbIndex) {
			Reply select (ReadRESP(*strm, yc));

			if (select.IsObjectType<RedisError>()) {
				// Likely NOAUTH or ERR DB
				BOOST_THROW_EXCEPTION(std::runtime_error(RedisError::Ptr(select)->GetMessage()));
			}
		}
	}
}

/**
 * Creates a Timeout which cancels stream's I/O after m_ConnectTimeout
 *
 * @param stream Redis server connection
 */
template<class StreamPtr>
Timeout::Ptr RedisConnection::MakeTimeout(StreamPtr& stream)
{
	Ptr keepAlive (this);

	return new Timeout(
		m_Strand.context(),
		m_Strand,
		boost::posix_time::microseconds(intmax_t(m_ConnectTimeout * 1000000)),
		[keepAlive, stream](boost::asio::yield_context yc) {
			boost::system::error_code ec;
			stream->lowest_layer().cancel(ec);
		}
	);
}

/**
 * Read a Redis protocol value from stream
 *
 * @param stream Redis server connection
 *
 * @return The value
 */
template<class AsyncReadStream>
Value RedisConnection::ReadRESP(AsyncReadStream& stream, boost::asio::yield_context& yc)
{
	namespace asio = boost::asio;

	char type = 0;
	asio::async_read(stream, asio::mutable_buffer(&type, 1), yc);

	switch (type) {
		case '+':
			{
				auto buf (ReadLine(stream, yc));
				return String(buf.begin(), buf.end());
			}
		case '-':
			{
				auto buf (ReadLine(stream, yc));
				return new RedisError(String(buf.begin(), buf.end()));
			}
		case ':':
			{
				auto buf (ReadLine(stream, yc, 21));
				intmax_t i = 0;

				try {
					i = boost::lexical_cast<intmax_t>(boost::string_view(buf.data(), buf.size()));
				} catch (...) {
					throw BadRedisInt(std::move(buf));
				}

				return (double)i;
			}
		case '$':
			{
				auto buf (ReadLine(stream, yc, 21));
				intmax_t i = 0;

				try {
					i = boost::lexical_cast<intmax_t>(boost::string_view(buf.data(), buf.size()));
				} catch (...) {
					throw BadRedisInt(std::move(buf));
				}

				if (i < 0) {
					return Value();
				}

				buf.clear();
				buf.insert(buf.end(), i, 0);
				asio::async_read(stream, asio::mutable_buffer(buf.data(), buf.size()), yc);

				{
					char crlf[2];
					asio::async_read(stream, asio::mutable_buffer(crlf, 2), yc);
				}

				return String(buf.begin(), buf.end());
			}
		case '*':
			{
				auto buf (ReadLine(stream, yc, 21));
				intmax_t i = 0;

				try {
					i = boost::lexical_cast<intmax_t>(boost::string_view(buf.data(), buf.size()));
				} catch (...) {
					throw BadRedisInt(std::move(buf));
				}

				if (i < 0) {
					return Empty;
				}

				Array::Ptr arr = new Array();

				arr->Reserve(i);

				for (; i; --i) {
					arr->Add(ReadRESP(stream, yc));
				}

				return arr;
			}
		default:
			throw BadRedisType(type);
	}
}

/**
 * Read from stream until \r\n
 *
 * @param stream Redis server connection
 * @param hint Expected amount of data
 *
 * @return Read data ex. \r\n
 */
template<class AsyncReadStream>
std::vector<char> RedisConnection::ReadLine(AsyncReadStream& stream, boost::asio::yield_context& yc, size_t hint)
{
	namespace asio = boost::asio;

	std::vector<char> line;
	line.reserve(hint);

	char next = 0;
	asio::mutable_buffer buf (&next, 1);

	for (;;) {
		asio::async_read(stream, buf, yc);

		if (next == '\r') {
			asio::async_read(stream, buf, yc);
			return line;
		}

		line.emplace_back(next);
	}
}

/**
 * Write a Redis protocol value to stream
 *
 * @param stream Redis server connection
 * @param query Redis protocol value
 */
template<class AsyncWriteStream>
void RedisConnection::WriteRESP(AsyncWriteStream& stream, const Query& query, boost::asio::yield_context& yc)
{
	namespace asio = boost::asio;

	asio::streambuf writeBuffer;
	std::ostream msg(&writeBuffer);

	msg << "*" << query.size() << "\r\n";

	for (auto& arg : query) {
		msg << "$" << arg.GetLength() << "\r\n" << arg << "\r\n";
	}

	asio::async_write(stream, writeBuffer, yc);
}

}

#endif //REDISCONNECTION_H
