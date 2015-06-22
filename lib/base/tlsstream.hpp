/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
class I2_BASE_API TlsStream : public Stream, private SocketEvents
{
public:
	DECLARE_PTR_TYPEDEFS(TlsStream);

	TlsStream(const Socket::Ptr& socket, const String& hostname, ConnectionRole role, const boost::shared_ptr<SSL_CTX>& sslContext);
	~TlsStream(void);

	boost::shared_ptr<X509> GetClientCertificate(void) const;
	boost::shared_ptr<X509> GetPeerCertificate(void) const;

	void Handshake(void);

	virtual void Close(void);
	virtual void Shutdown(void);

	virtual size_t Peek(void *buffer, size_t count, bool allow_partial = false);
	virtual size_t Read(void *buffer, size_t count, bool allow_partial = false);
	virtual void Write(const void *buffer, size_t count);

	virtual bool IsEof(void) const;

	virtual bool SupportsWaiting(void) const;
	virtual bool IsDataAvailable(void) const;

	bool IsVerifyOK(void) const;

private:
	boost::shared_ptr<SSL> m_SSL;
	bool m_Eof;
	mutable boost::mutex m_Mutex;
	mutable boost::condition_variable m_CV;
	bool m_HandshakeOK;
	bool m_VerifyOK;
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

	virtual void OnEvent(int revents);

	void HandleError(void) const;

	static int ValidateCertificate(int preverify_ok, X509_STORE_CTX *ctx);
	static void NullCertificateDeleter(X509 *certificate);
};

}

#endif /* TLSSTREAM_H */
