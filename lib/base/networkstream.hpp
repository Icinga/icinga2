/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NETWORKSTREAM_H
#define NETWORKSTREAM_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"
#include "base/socket.hpp"

namespace icinga
{

/**
 * A network stream. DEPRECATED - Use Boost ASIO instead.
 *
 * @ingroup base
 */
class NetworkStream final : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(NetworkStream);

	NetworkStream(Socket::Ptr socket);

	size_t Read(void *buffer, size_t count) override;
	void Write(const void *buffer, size_t count) override;

	void Close() override;

	bool IsEof() const override;

private:
	Socket::Ptr m_Socket;
	bool m_Eof;
};

}

#endif /* NETWORKSTREAM_H */
