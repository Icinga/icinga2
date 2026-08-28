// SPDX-FileCopyrightText: 2019 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifndef _WIN32

#include "base/object.hpp"
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/spawn.hpp>
#include <thread>

namespace icinga
{

/**
 * @ingroup cli
 */
class DaemonControl : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DaemonControl);

	inline DaemonControl()
		: m_IO(), m_KeepAlive(boost::asio::make_work_guard(m_IO)), m_Acceptor(m_IO), m_WasRunningBeforeFork(false)
	{
	}

	DaemonControl(const DaemonControl&) = delete;
	DaemonControl(DaemonControl&&) = delete;
	DaemonControl& operator=(const DaemonControl&) = delete;
	DaemonControl& operator=(DaemonControl&&) = delete;

	~DaemonControl();

	void Start();
	void Stop();
	void BeforeFork();
	void AfterFork(bool parent);
private:
	typedef boost::asio::local::stream_protocol UnixStream;
	typedef boost::asio::buffered_stream<UnixStream::socket> BufferedUnixStream;

	class Connection : public Object
	{
	public:
		DECLARE_PTR_TYPEDEFS(Connection);

		inline Connection(boost::asio::io_context& io) : m_Peer(io)
		{
		}

		Connection(const Connection&) = delete;
		Connection(Connection&&) = delete;
		Connection& operator=(const Connection&) = delete;
		Connection& operator=(Connection&&) = delete;

		inline BufferedUnixStream::lowest_layer_type& GetSocket()
		{
			return m_Peer.lowest_layer();
		}

		void Start();

	private:
		BufferedUnixStream m_Peer;

		void ProcessMessages(boost::asio::yield_context& yc);
	};

	boost::asio::io_context m_IO;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_KeepAlive;
	UnixStream::acceptor m_Acceptor;
	std::thread m_Thread;
	bool m_WasRunningBeforeFork;

	void RunEventLoop();
	void RunAcceptLoop(boost::asio::yield_context& yc);
};

}

#endif /* _WIN32 */
