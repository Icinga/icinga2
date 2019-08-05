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

#include "redis/redisconnection.hpp"
#include "base/array.hpp"
#include "base/convert.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/string.hpp"
#include "base/tcpsocket.hpp"
#include <boost/asio/post.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/variant/get.hpp>
#include <iterator>
#include <memory>
#include <utility>

using namespace icinga;
namespace asio = boost::asio;

RedisConnection::RedisConnection(const String host, const int port, const String path, const String password, const int db) :
	RedisConnection(IoEngine::Get().GetIoService(), host, port, path, password, db)
{
}

RedisConnection::RedisConnection(boost::asio::io_context& io, String host, int port, String path, String password, int db)
	: m_Host(std::move(host)), m_Port(port), m_Path(std::move(path)), m_Password(std::move(password)), m_DbIndex(db),
	  m_Connecting(false), m_Connected(false), m_Strand(io), m_QueuedWrites(io), m_QueuedReads(io)
{
}

void RedisConnection::Start()
{
	if (!m_Connecting.exchange(true)) {
		Ptr keepAlive (this);

		asio::spawn(m_Strand, [this, keepAlive](asio::yield_context yc) { Connect(yc); });
	}
}

bool RedisConnection::IsConnected() {
	return m_Connected.load();
}

void RedisConnection::FireAndForgetQuery(RedisConnection::Query query)
{
	auto item (std::make_shared<decltype(m_Queues.FireAndForgetQuery)::value_type>(std::move(query)));

	asio::post(m_Strand, [this, item]() {
		m_Queues.FireAndForgetQuery.emplace(std::move(*item));
		m_QueuedWrites.Set();
	});
}

void RedisConnection::FireAndForgetQueries(RedisConnection::Queries queries)
{
	auto item (std::make_shared<decltype(m_Queues.FireAndForgetQueries)::value_type>(std::move(queries)));

	asio::post(m_Strand, [this, item]() {
		m_Queues.FireAndForgetQueries.emplace(std::move(*item));
		m_QueuedWrites.Set();
	});
}

RedisConnection::Reply RedisConnection::GetResultOfQuery(RedisConnection::Query query)
{
	std::promise<Reply> promise;
	auto future (promise.get_future());
	auto item (std::make_shared<decltype(m_Queues.GetResultOfQuery)::value_type>(std::move(query), std::move(promise)));

	asio::post(m_Strand, [this, item]() {
		m_Queues.GetResultOfQuery.emplace(std::move(*item));
		m_QueuedWrites.Set();
	});

	item = nullptr;
	future.wait();
	return future.get();
}

RedisConnection::Replies RedisConnection::GetResultsOfQueries(RedisConnection::Queries queries)
{
	std::promise<Replies> promise;
	auto future (promise.get_future());
	auto item (std::make_shared<decltype(m_Queues.GetResultsOfQueries)::value_type>(std::move(queries), std::move(promise)));

	asio::post(m_Strand, [this, item]() {
		m_Queues.GetResultsOfQueries.emplace(std::move(*item));
		m_QueuedWrites.Set();
	});

	item = nullptr;
	future.wait();
	return future.get();
}

void RedisConnection::Connect(asio::yield_context& yc)
{
	Defer notConnecting ([this]() {
		if (!m_Connected.load()) {
			m_Connecting.store(false);
		}
	});

	Log(LogInformation, "RedisWriter", "Trying to connect to Redis server (async)");

	try {
		if (m_Path.IsEmpty()) {
			m_TcpConn = decltype(m_TcpConn)(new TcpConn(m_Strand.context()));
			icinga::Connect(m_TcpConn->next_layer(), m_Host, Convert::ToString(m_Port), yc);
		} else {
			m_UnixConn = decltype(m_UnixConn)(new UnixConn(m_Strand.context()));
			m_UnixConn->next_layer().async_connect(Unix::endpoint(m_Path.CStr()), yc);
		}

		{
			Ptr keepAlive (this);

			asio::spawn(m_Strand, [this, keepAlive](asio::yield_context yc) { ReadLoop(yc); });
			asio::spawn(m_Strand, [this, keepAlive](asio::yield_context yc) { WriteLoop(yc); });
		}

		m_Connected.store(true);
	} catch (const boost::coroutines::detail::forced_unwind&) {
		throw;
	} catch (const std::exception& ex) {
		Log(LogCritical, "RedisWriter")
			<< "Cannot connect to " << m_Host << ":" << m_Port << ": " << ex.what();
	}
}

void RedisConnection::ReadLoop(asio::yield_context& yc)
{
	for (;;) {
		m_QueuedReads.Wait(yc);

		do {
			auto item (std::move(m_Queues.FutureResponseActions.front()));
			m_Queues.FutureResponseActions.pop();

			switch (item.Action) {
				case ResponseAction::Ignore:
					for (auto i (item.Amount); i; --i) {
						ReadOne(yc);
					}
					break;
				case ResponseAction::Deliver:
					for (auto i (item.Amount); i; --i) {
						auto promise (std::move(m_Queues.ReplyPromises.front()));
						m_Queues.ReplyPromises.pop();

						promise.set_value(ReadOne(yc));
					}
					break;
				case ResponseAction::DeliverBulk:
					{
						auto promise (std::move(m_Queues.RepliesPromises.front()));
						m_Queues.RepliesPromises.pop();

						Replies replies;
						replies.reserve(item.Amount);

						for (auto i (item.Amount); i; --i) {
							replies.emplace_back(ReadOne(yc));
						}

						promise.set_value(std::move(replies));
					}
			}
		} while (!m_Queues.FutureResponseActions.empty());

		m_QueuedReads.Clear();
	}
}

void RedisConnection::WriteLoop(asio::yield_context& yc)
{
	for (;;) {
		m_QueuedWrites.Wait(yc);

		bool writtenAll = true;

		do {
			writtenAll = true;

			if (!m_Queues.FireAndForgetQuery.empty()) {
				auto item (std::move(m_Queues.FireAndForgetQuery.front()));
				m_Queues.FireAndForgetQuery.pop();

				if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Ignore) {
					m_Queues.FutureResponseActions.emplace(FutureResponseAction{1, ResponseAction::Ignore});
				} else {
					++m_Queues.FutureResponseActions.back().Amount;
				}

				m_QueuedReads.Set();
				writtenAll = false;

				WriteOne(item, yc);
			}

			if (!m_Queues.FireAndForgetQueries.empty()) {
				auto item (std::move(m_Queues.FireAndForgetQueries.front()));
				m_Queues.FireAndForgetQueries.pop();

				if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Ignore) {
					m_Queues.FutureResponseActions.emplace(FutureResponseAction{item.size(), ResponseAction::Ignore});
				} else {
					m_Queues.FutureResponseActions.back().Amount += item.size();
				}

				m_QueuedReads.Set();
				writtenAll = false;

				for (auto& query : item) {
					WriteOne(query, yc);
				}
			}

			if (!m_Queues.GetResultOfQuery.empty()) {
				auto item (std::move(m_Queues.GetResultOfQuery.front()));
				m_Queues.GetResultOfQuery.pop();
				m_Queues.ReplyPromises.emplace(std::move(item.second));

				if (m_Queues.FutureResponseActions.empty() || m_Queues.FutureResponseActions.back().Action != ResponseAction::Deliver) {
					m_Queues.FutureResponseActions.emplace(FutureResponseAction{1, ResponseAction::Deliver});
				} else {
					++m_Queues.FutureResponseActions.back().Amount;
				}

				m_QueuedReads.Set();
				writtenAll = false;

				WriteOne(item.first, yc);
			}

			if (!m_Queues.GetResultsOfQueries.empty()) {
				auto item (std::move(m_Queues.GetResultsOfQueries.front()));
				m_Queues.GetResultsOfQueries.pop();
				m_Queues.RepliesPromises.emplace(std::move(item.second));
				m_Queues.FutureResponseActions.emplace(FutureResponseAction{item.first.size(), ResponseAction::DeliverBulk});

				m_QueuedReads.Set();
				writtenAll = false;

				for (auto& query : item.first) {
					WriteOne(query, yc);
				}
			}
		} while (!writtenAll);

		m_QueuedWrites.Clear();

		if (m_Path.IsEmpty()) {
			m_TcpConn->async_flush(yc);
		} else {
			m_UnixConn->async_flush(yc);
		}

	}
}

RedisConnection::Reply RedisConnection::ReadOne(boost::asio::yield_context& yc)
{
	if (m_Path.IsEmpty()) {
		return ReadRESP(*m_TcpConn, yc);
	} else {
		return ReadRESP(*m_UnixConn, yc);
	}
}

void RedisConnection::WriteOne(RedisConnection::Query& query, asio::yield_context& yc)
{
	if (m_Path.IsEmpty()) {
		WriteRESP(*m_TcpConn, query, yc);
	} else {
		WriteRESP(*m_UnixConn, query, yc);
	}
}
