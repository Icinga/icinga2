// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef UNIXSOCKET_H
#define UNIXSOCKET_H

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

#endif /* UNIXSOCKET_H */
