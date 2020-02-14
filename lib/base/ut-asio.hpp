/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_ASIO_H
#define UT_ASIO_H

#include <cstdint>
#include <utility>

#ifdef _WIN32
#	include <winsock2.h>
#else /* _WIN32 */
#	include <sys/socket.h>
#endif /* _WIN32 */

namespace icinga
{
namespace UT
{

typedef decltype(socket(0, 0, 0)) NativeSocket;

enum class SocketOp : uint_fast8_t
{
	Read, Write
};

void WaitForSocket(NativeSocket sock, SocketOp op);

namespace Aware
{

/**
 * Like SRS, but UserspaceThread-aware.
 *
 * @ingroup base
 */
template<class SRS>
class SyncReadStream : public SRS
{
public:
	using SRS::SRS;

	template<class... Args>
	auto read_some(Args&&... args) -> decltype(((SRS*)nullptr)->read_some(std::forward<Args>(args)...))
	{
		WaitForSocket(this->lowest_layer().native_handle(), SocketOp::Read);

		return ((SRS*)this)->read_some(std::forward<Args>(args)...);
	}
};

/**
 * Like SWS, but UserspaceThread-aware.
 *
 * @ingroup base
 */
template<class SWS>
class SyncWriteStream : public SWS
{
public:
	using SWS::SWS;

	template<class... Args>
	auto write_some(Args&&... args) -> decltype(((SWS*)nullptr)->write_some(std::forward<Args>(args)...))
	{
		WaitForSocket(this->lowest_layer().native_handle(), SocketOp::Write);

		return ((SWS*)this)->write_some(std::forward<Args>(args)...);
	}
};

}
}
}

#endif /* UT_ASIO_H */
