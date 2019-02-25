/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"

namespace icinga
{

/**
 * A TCP socket.
 *
 * @ingroup base
 */
class TcpSocket final : public Socket
{
public:
	DECLARE_PTR_TYPEDEFS(TcpSocket);

	void Bind(const String& service, int family);
	void Bind(const String& node, const String& service, int family);

	void Connect(const String& node, const String& service);
};

}

#endif /* TCPSOCKET_H */
