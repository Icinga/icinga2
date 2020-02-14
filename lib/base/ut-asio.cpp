/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/exception.hpp"
#include "base/socket.hpp"
#include "base/ut-asio.hpp"
#include "base/ut-current.hpp"

#ifdef _WIN32
#	include <winsock.h>
#	include <winsock2.h>
#else /* _WIN32 */
#	include <errno.h>
#	include <sys/select.h>
#endif /* _WIN32 */

using namespace icinga;

void UT::WaitForSocket(UT::NativeSocket sock, UT::SocketOp op)
{
	fd_set fds;
	fd_set* reads = nullptr;
	fd_set* writes = nullptr;
	struct timeval timeout = { 0, 0 };

	switch (op) {
		case SocketOp::Read:
			reads = &fds;
			break;
		case SocketOp::Write:
			writes = &fds;
	}

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		auto readyFds (select(1, reads, writes, nullptr, &timeout));

		if (readyFds < 0u) {
#ifdef _WIN32
			BOOST_THROW_EXCEPTION(socket_error()
				<< boost::errinfo_api_function("select")
				<< errinfo_win32_error(WSAGetLastError()));
#else /* _WIN32 */
			BOOST_THROW_EXCEPTION(socket_error()
				<< boost::errinfo_api_function("select")
				<< boost::errinfo_errno(errno));
#endif /* _WIN32 */
		}

		if (readyFds) {
			break;
		}

		Current::Yield_();
	}
}
