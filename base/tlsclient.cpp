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

#include "i2-base.h"

using namespace icinga;

int I2_EXPORT TlsClient::m_SSLIndex;
bool I2_EXPORT TlsClient::m_SSLIndexInitialized = false;

/**
 * Constructor for the TlsClient class.
 *
 * @param role The role of the client.
 * @param sslContext The SSL context for the client.
 */
TlsClient::TlsClient(TcpClientRole role, shared_ptr<SSL_CTX> sslContext) : TcpClient(role)
{
	m_SSLContext = sslContext;
	m_BlockRead = false;
	m_BlockWrite = false;
}

/**
 * Takes a certificate as an argument. Does nothing.
 *
 * @param certificate An X509 certificate.
 */
void TlsClient::NullCertificateDeleter(X509 *certificate)
{
	/* Nothing to do here. */
}

/**
 * Retrieves the X509 certficate for this client.
 *
 * @returns The X509 certificate.
 */
shared_ptr<X509> TlsClient::GetClientCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_certificate(m_SSL.get()), &TlsClient::NullCertificateDeleter);
}

/**
 * Retrieves the X509 certficate for the peer.
 *
 * @returns The X509 certificate.
 */
shared_ptr<X509> TlsClient::GetPeerCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_peer_certificate(m_SSL.get()), X509_free);
}

/**
 * Registers the TLS socket and starts processing events for it.
 */
void TlsClient::Start(void)
{
	TcpClient::Start();

	m_SSL = shared_ptr<SSL>(SSL_new(m_SSLContext.get()), SSL_free);

	if (!m_SSL)
		throw OpenSSLException("SSL_new failed", ERR_get_error());

	if (!GetClientCertificate())
		throw logic_error("No X509 client certificate was specified.");

	if (!m_SSLIndexInitialized) {
		m_SSLIndex = SSL_get_ex_new_index(0, (void *)"TlsClient", NULL, NULL, NULL);
		m_SSLIndexInitialized = true;
	}

	SSL_set_ex_data(m_SSL.get(), m_SSLIndex, this);

	SSL_set_verify(m_SSL.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
	    &TlsClient::SSLVerifyCertificate);

	BIO *bio = BIO_new_socket(GetFD(), 0);
	SSL_set_bio(m_SSL.get(), bio, bio);

	if (GetRole() == RoleInbound)
		SSL_set_accept_state(m_SSL.get());
	else
		SSL_set_connect_state(m_SSL.get());

	SSL_do_handshake(m_SSL.get());
}

/**
 * Processes data that is available for this socket.
 */
size_t TlsClient::FillRecvQueue(void)
{
	int result;

	m_BlockRead = false;
	m_BlockWrite = false;

	result = 0;

	for (;;) {
		int rc;
		size_t bufferSize = FIFO::BlockSize / 2;
		char *buffer = (char *)GetRecvQueue()->GetWriteBuffer(&bufferSize);
		rc = SSL_read(m_SSL.get(), buffer, bufferSize);

		if (rc <= 0) {
			switch (SSL_get_error(m_SSL.get(), rc)) {
				case SSL_ERROR_WANT_WRITE:
					m_BlockRead = true;
					/* fall through */
				case SSL_ERROR_WANT_READ:
					return result;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return result;
				default:
					HandleSocketError(OpenSSLException(
					    "SSL_read failed", ERR_get_error()));
					return result;
			}
		}

		GetRecvQueue()->Write(NULL, rc);

		result += rc;
	}

	return result;
}

/**
 * Processes data that can be written for this socket.
 */
size_t TlsClient::FlushSendQueue(void)
{
	int rc;

	if (!WantsToWrite())
		return 0;

	std::cerr << "tls sendq: " << GetSendQueue()->GetSize() << " bytes" << std::endl;

	m_BlockRead = false;
	m_BlockWrite = false;

	rc = SSL_write(m_SSL.get(), (const char *)GetSendQueue()->GetReadBuffer(), GetSendQueue()->GetSize());

	if (rc <= 0) {
		switch (SSL_get_error(m_SSL.get(), rc)) {
			case SSL_ERROR_WANT_READ:
				m_BlockWrite = true;
				/* fall through */
			case SSL_ERROR_WANT_WRITE:
				return 0;
			case SSL_ERROR_ZERO_RETURN:
				Close();
				return 0;
			default:
				HandleSocketError(OpenSSLException(
				    "SSL_write failed", ERR_get_error()));
				return 0;
		}
	}

	GetSendQueue()->Read(NULL, rc);

	return rc;
}

/**
 * Checks whether data should be read for this socket.
 *
 * @returns true if data should be read, false otherwise.
 */
bool TlsClient::WantsToRead(void) const
{
	if (SSL_want_read(m_SSL.get()))
		return true;

	if (m_BlockRead)
		return false;

	return TcpClient::WantsToRead();
}

/**
 * Checks whether data should be written for this socket.
 *
 * @returns true if data should be written, false otherwise.
 */
bool TlsClient::WantsToWrite(void) const
{
	if (SSL_want_write(m_SSL.get()))
		return true;

	if (m_BlockWrite)
		return false;

	return TcpClient::WantsToWrite();
}

/**
 * Closes the socket.
 *
 * @param from_dtor Whether this method was invoked from the destructor.
 */
void TlsClient::CloseInternal(bool from_dtor)
{
	if (m_SSL)
		SSL_shutdown(m_SSL.get());

	TcpClient::CloseInternal(from_dtor);
}

/**
 * Factory function for the TlsClient class.
 *
 * @param role The role of the TLS socket.
 * @param sslContext The SSL context for the socket.
 * @returns A new TLS socket.
 */
TcpClient::Ptr icinga::TlsClientFactory(TcpClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	return boost::make_shared<TlsClient>(role, sslContext);
}

/**
 * Callback function that verifies SSL certificates.
 *
 * @param ok Whether pre-checks for the SSL certificates were successful.
 * @param x509Context X509 context for the certificate.
 * @returns 1 if the verification was successful, 0 otherwise.
 */
int TlsClient::SSLVerifyCertificate(int ok, X509_STORE_CTX *x509Context)
{
	SSL *ssl = (SSL *)X509_STORE_CTX_get_ex_data(x509Context, SSL_get_ex_data_X509_STORE_CTX_idx());
	TlsClient *client = (TlsClient *)SSL_get_ex_data(ssl, m_SSLIndex);

	if (client == NULL)
		return 0;

	bool valid = (ok != 0);
	shared_ptr<X509> x509Certificate = shared_ptr<X509>(x509Context->cert, &TlsClient::NullCertificateDeleter);
	client->OnVerifyCertificate(client->GetSelf(), &valid, x509Context, x509Certificate);

	return valid ? 1 : 0;
}
