/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SOCKET_H
#define SOCKET_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <mutex>

namespace icinga
{

/**
 * Base class for connection-oriented sockets.
 *
 * @ingroup base
 */
class Socket : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Socket);

	Socket() = default;
	Socket(SOCKET fd);
	~Socket() override;

	SOCKET GetFD() const;

	void Close();

	std::pair<String, String> GetClientAddressDetails();
	String GetClientAddress();
	std::pair<String, String> GetPeerAddressDetails();
	String GetPeerAddress();

	size_t Read(void *buffer, size_t size);
	size_t Write(const void *buffer, size_t size);

	void Listen();
	Socket::Ptr Accept();

	bool Poll(bool read, bool write, struct timeval *timeout = nullptr);

	void MakeNonBlocking();

	static void SocketPair(SOCKET s[2]);

protected:
	void SetFD(SOCKET fd);

	int GetError() const;

	mutable std::mutex m_SocketMutex;

private:
	SOCKET m_FD{INVALID_SOCKET}; /**< The socket descriptor. */

	static std::pair<String, String> GetDetailsFromSockaddr(sockaddr *address, socklen_t len);
	static String GetHumanReadableAddress(const std::pair<String, String>& socketDetails);
};

class socket_error : virtual public std::exception, virtual public boost::exception { };

}

#endif /* SOCKET_H */
