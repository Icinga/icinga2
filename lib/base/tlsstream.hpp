/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TLSSTREAM_H
#define TLSSTREAM_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"
#include "base/socketevents.hpp"
#include "base/stream.hpp"
#include "base/tlsutility.hpp"
#include "base/fifo.hpp"
#include <utility>
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace icinga
{

enum TlsAction
{
	TlsActionNone,
	TlsActionRead,
	TlsActionWrite,
	TlsActionHandshake
};

/**
 * A TLS stream.
 *
 * @ingroup base
 */
class TlsStream final : public SocketEvents
{
public:
	DECLARE_PTR_TYPEDEFS(TlsStream);

	TlsStream(const Socket::Ptr& socket, const String& hostname, ConnectionRole role, const std::shared_ptr<SSL_CTX>& sslContext = MakeSSLContext());
	TlsStream(const Socket::Ptr& socket, const String& hostname, ConnectionRole role, const std::shared_ptr<boost::asio::ssl::context>& sslContext);
	~TlsStream() override;

	Socket::Ptr GetSocket() const;

	std::shared_ptr<X509> GetClientCertificate() const;
	std::shared_ptr<X509> GetPeerCertificate() const;

	void Handshake();

	void Close() override;
	void Shutdown() override;

	size_t Peek(void *buffer, size_t count, bool allow_partial = false) override;
	size_t Read(void *buffer, size_t count, bool allow_partial = false) override;
	void Write(const void *buffer, size_t count) override;

	bool IsEof() const override;

	bool SupportsWaiting() const override;
	bool IsDataAvailable() const override;

	bool IsVerifyOK() const;
	String GetVerifyError() const;

private:
	std::shared_ptr<SSL> m_SSL;
	bool m_Eof;
	mutable boost::mutex m_Mutex;
	mutable boost::condition_variable m_CV;
	bool m_HandshakeOK;
	bool m_VerifyOK;
	String m_VerifyError;
	int m_ErrorCode;
	bool m_ErrorOccurred;

	Socket::Ptr m_Socket;
	ConnectionRole m_Role;

	FIFO::Ptr m_SendQ;
	FIFO::Ptr m_RecvQ;

	TlsAction m_CurrentAction;
	bool m_Retry;
	bool m_Shutdown;

	static int m_SSLIndex;
	static bool m_SSLIndexInitialized;

	TlsStream(const Socket::Ptr& socket, const String& hostname, ConnectionRole role, SSL_CTX* sslContext);

	void OnEvent(int revents) override;

	void HandleError() const;

	static int ValidateCertificate(int preverify_ok, X509_STORE_CTX *ctx);
	static void NullCertificateDeleter(X509 *certificate);

	void CloseInternal(bool inDestructor);
};

class UnbufferedAsioTlsStream : public boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
{
public:
	inline
	UnbufferedAsioTlsStream(std::pair<boost::asio::io_service*, boost::asio::ssl::context*>& init)
		: stream(*init.first, *init.second)
	{
	}
};

class AsioTlsStream : public boost::asio::buffered_stream<UnbufferedAsioTlsStream>
{
public:
	inline
	AsioTlsStream(boost::asio::io_service& ioService, boost::asio::ssl::context& sslContext)
		: AsioTlsStream(std::make_pair(&ioService, &sslContext))
	{
	}

private:
	inline
	AsioTlsStream(std::pair<boost::asio::io_service*, boost::asio::ssl::context*> init)
		: buffered_stream(init)
	{
	}
};

}

#endif /* TLSSTREAM_H */
