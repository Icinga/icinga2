/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef _WIN32

#ifndef DAEMONCONTROL_H
#define DAEMONCONTROL_H

#include "base/object.hpp"
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace icinga
{

/**
 * @ingroup cli
 */
class DaemonControl : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DaemonControl);

	typedef std::function<void(bool)> ReloadResultHandler;

	inline DaemonControl()
		: m_IO(), m_KeepAlive(m_IO), m_AlreadyExpiredTimer(m_IO), m_Acceptor(m_IO), m_WasRunningBeforeFork(false)
	{
		m_AlreadyExpiredTimer.expires_at(boost::posix_time::neg_infin);
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

	std::vector<ReloadResultHandler> PopPendingReloadResultHandlers();
private:
	typedef boost::asio::local::stream_protocol UnixStream;
	typedef boost::asio::buffered_stream<UnixStream::socket> BufferedUnixStream;

	class Connection : public Object
	{
	public:
		DECLARE_PTR_TYPEDEFS(Connection);

		inline Connection(DaemonControl& dc) : m_Peer(dc.m_IO), m_DaemonControl(dc)
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
		typedef boost::beast::http::parser<true, boost::beast::http::string_body> RequestParser;
		typedef boost::beast::http::request<boost::beast::http::string_body> Request;
		typedef boost::beast::http::response<boost::beast::http::string_body> Response;

		BufferedUnixStream m_Peer;
		boost::beast::flat_buffer m_Buf;
		DaemonControl& m_DaemonControl;

		void ProcessMessages(boost::asio::yield_context& yc);

		inline bool EnsureValidHeaders(RequestParser& parser, Response& response, boost::asio::yield_context& yc);
		inline bool EnsureValidBody(RequestParser& parser, Response& response, boost::asio::yield_context& yc);
		inline void ProcessRequest(Request& request, Response& response, boost::asio::yield_context& yc);

		inline bool HandleV1Reload(Request& request, Response& response, boost::asio::yield_context& yc);
	};

	friend Connection;

	boost::asio::io_context m_IO;
	boost::asio::io_context::work m_KeepAlive;
	boost::asio::deadline_timer m_AlreadyExpiredTimer;
	UnixStream::acceptor m_Acceptor;
	std::thread m_Thread;
	bool m_WasRunningBeforeFork;
	std::vector<ReloadResultHandler> m_PendingReloadResultHandlers;
	std::mutex m_PendingReloadResultHandlersMutex;

	void RunEventLoop();
	void RunAcceptLoop(boost::asio::yield_context& yc);
};

}

#endif /* DAEMONCONTROL_H */

#endif /* _WIN32 */
