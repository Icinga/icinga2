/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/socket.hpp"

#ifndef _WIN32
namespace icinga
{

/**
 * A TCP socket. DEPRECATED - Use Boost ASIO instead.
 *
 * @ingroup base
 */
class UnixSocket final : public Socket
{
public:
	DECLARE_PTR_TYPEDEFS(UnixSocket);

	UnixSocket();

	void Bind(const String& path);

	void Connect(const String& path);
};

}
#endif /* _WIN32 */
