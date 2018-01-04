/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef TLSSTREAM_H
#define TLSSTREAM_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"
#include "base/socketevents.hpp"
#include "base/stream.hpp"
#include "base/tlsutility.hpp"
#include "base/fifo.hpp"

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
class TlsStream final : public Stream, private SocketEvents
{
public:
	DECLARE_PTR_TYPEDEFS(TlsStream);

	TlsStream(const Socket::Ptr& socket, const String& hostname, ConnectionRole role, const std::shared_ptr<SSL_CTX>& sslContext = MakeSSLContext());
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

	void OnEvent(int revents) override;

	void HandleError() const;

	static int ValidateCertificate(int preverify_ok, X509_STORE_CTX *ctx);
	static void NullCertificateDeleter(X509 *certificate);

	void CloseInternal(bool inDestructor);
};

}

#endif /* TLSSTREAM_H */
