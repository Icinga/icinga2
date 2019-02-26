/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef UNIXSOCKET_H
#define UNIXSOCKET_H

#include "base/socket.hpp"

#ifndef _WIN32
namespace icinga
{

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
