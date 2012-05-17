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

#ifndef TLSCLIENT_H
#define TLSCLIENT_H

namespace icinga
{

/**
 * Event arguments for the "SSL certificate verification" event.
 */
struct I2_BASE_API VerifyCertificateEventArgs : public EventArgs
{
	bool ValidCertificate; /**< Whether the certificate is valid, can be
			            changed by the event handler. */
	X509_STORE_CTX *Context; /**< The X509 store context. */
	shared_ptr<X509> Certificate; /**< The X509 certificate that should
				           ve verified. */
};

/**
 * A TLS client connection.
 */
class I2_BASE_API TLSClient : public TCPClient
{
private:
	shared_ptr<SSL_CTX> m_SSLContext;
	shared_ptr<SSL> m_SSL;

	bool m_BlockRead;
	bool m_BlockWrite;

	static int m_SSLIndex;
	static bool m_SSLIndexInitialized;

	virtual int ReadableEventHandler(const EventArgs& ea);
	virtual int WritableEventHandler(const EventArgs& ea);

	virtual void CloseInternal(bool from_dtor);

	static void NullCertificateDeleter(X509 *certificate);

	static int SSLVerifyCertificate(int ok, X509_STORE_CTX *x509Context);

protected:
	void HandleSSLError(void);

public:
	TLSClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext);

	shared_ptr<X509> GetClientCertificate(void) const;
	shared_ptr<X509> GetPeerCertificate(void) const;

	virtual void Start(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	Observable<VerifyCertificateEventArgs> OnVerifyCertificate;
};

TCPClient::Ptr TLSClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext);

}

#endif /* TLSCLIENT_H */
