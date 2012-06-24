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
 * A TLS client connection.
 *
 * @ingroup base
 */
class I2_BASE_API TlsClient : public TcpClient
{
public:
	TlsClient(TcpClientRole role, shared_ptr<SSL_CTX> sslContext);

	virtual void Start(void);

	shared_ptr<X509> GetClientCertificate(void) const;
	shared_ptr<X509> GetPeerCertificate(void) const;

	boost::signal<void (const TlsClient::Ptr&)> OnCertificateValidated;

protected:
	void HandleSSLError(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void HandleReadable(void);
	virtual void HandleWritable(void);

	virtual bool ValidateCertificate(bool ok, X509_STORE_CTX *x509Context, const shared_ptr<X509>& x509Certificate);

private:
	shared_ptr<SSL_CTX> m_SSLContext;
	shared_ptr<SSL> m_SSL;

	bool m_BlockRead;
	bool m_BlockWrite;

	static int m_SSLIndex;
	static bool m_SSLIndexInitialized;

	virtual void CloseInternal(bool from_dtor);

	static void NullCertificateDeleter(X509 *certificate);

	static int SSLVerifyCertificate(int ok, X509_STORE_CTX *x509Context);
	int ValidateCertificateInternal(int ok, X509_STORE_CTX *x509Context);
};

TcpClient::Ptr TlsClientFactory(TcpClientRole role, shared_ptr<SSL_CTX> sslContext);

}

#endif /* TLSCLIENT_H */
