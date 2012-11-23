/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

namespace icinga
{

enum TlsRole
{
	TlsRoleClient,
	TlsRoleServer
};

/**
 * A TLS stream.
 *
 * @ingroup base
 */
class I2_BASE_API TlsStream : public Stream
{
public:
	typedef shared_ptr<TlsStream> Ptr;
	typedef weak_ptr<TlsStream> WeakPtr;

	TlsStream(const Stream::Ptr& innerStream, TlsRole role, shared_ptr<SSL_CTX> sslContext);

	shared_ptr<X509> GetClientCertificate(void) const;
	shared_ptr<X509> GetPeerCertificate(void) const;

	virtual void Start(void);
	virtual void Close(void);

	virtual size_t GetAvailableBytes(void) const;
	virtual size_t Peek(void *buffer, size_t count);
	virtual size_t Read(void *buffer, size_t count);
	virtual void Write(const void *buffer, size_t count);

private:
	shared_ptr<SSL_CTX> m_SSLContext;
	shared_ptr<SSL> m_SSL;
	BIO *m_BIO;

	FIFO::Ptr m_SendQueue;
	FIFO::Ptr m_RecvQueue;

	Stream::Ptr m_InnerStream;
	TlsRole m_Role;

	static int m_SSLIndex;
	static bool m_SSLIndexInitialized;

	void DataAvailableHandler(void);
	void ClosedHandler(void);

	void HandleIO(void);

	static void NullCertificateDeleter(X509 *certificate);
};

}

#endif /* TLSSTREAM_H */
