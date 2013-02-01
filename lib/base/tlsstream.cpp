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

int I2_EXPORT TlsStream::m_SSLIndex;
bool I2_EXPORT TlsStream::m_SSLIndexInitialized = false;

/**
 * Constructor for the TlsStream class.
 *
 * @param role The role of the client.
 * @param sslContext The SSL context for the client.
 */
TlsStream::TlsStream(const Stream::Ptr& innerStream, TlsRole role, shared_ptr<SSL_CTX> sslContext)
	: m_InnerStream(innerStream), m_SSLContext(sslContext), m_Role(role),
	  m_SendQueue(boost::make_shared<FIFO>()), m_RecvQueue(boost::make_shared<FIFO>())
{
	m_InnerStream->OnDataAvailable.connect(boost::bind(&TlsStream::DataAvailableHandler, this));
	m_InnerStream->OnClosed.connect(boost::bind(&TlsStream::ClosedHandler, this));
	m_SendQueue->Start();
	m_RecvQueue->Start();
}

void TlsStream::Start(void)
{
	m_SSL = shared_ptr<SSL>(SSL_new(m_SSLContext.get()), SSL_free);

	m_SSLContext.reset();

	if (!m_SSL)
		throw_exception(OpenSSLException("SSL_new failed", ERR_get_error()));

	if (!GetClientCertificate())
		throw_exception(logic_error("No X509 client certificate was specified."));

	if (!m_SSLIndexInitialized) {
		m_SSLIndex = SSL_get_ex_new_index(0, const_cast<char *>("TlsStream"), NULL, NULL, NULL);
		m_SSLIndexInitialized = true;
	}

	SSL_set_ex_data(m_SSL.get(), m_SSLIndex, this);

	SSL_set_verify(m_SSL.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	m_BIO = BIO_new_I2Stream(m_InnerStream);
	SSL_set_bio(m_SSL.get(), m_BIO, m_BIO);

	if (m_Role == TlsRoleServer)
		SSL_set_accept_state(m_SSL.get());
	else
		SSL_set_connect_state(m_SSL.get());

	/*int rc = SSL_do_handshake(m_SSL.get());

	if (rc == 1) {
		SetConnected(true);
		OnConnected(GetSelf());
	}*/

	Stream::Start();

	HandleIO();
}

/**
 * Retrieves the X509 certficate for this client.
 *
 * @returns The X509 certificate.
 */
shared_ptr<X509> TlsStream::GetClientCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_certificate(m_SSL.get()), &Utility::NullDeleter);
}

/**
 * Retrieves the X509 certficate for the peer.
 *
 * @returns The X509 certificate.
 */
shared_ptr<X509> TlsStream::GetPeerCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_peer_certificate(m_SSL.get()), X509_free);
}

void TlsStream::DataAvailableHandler(void)
{
	try {
		HandleIO();
	} catch (...) {
		SetException(boost::current_exception());

		Close();
	}
}

void TlsStream::ClosedHandler(void)
{
	SetException(m_InnerStream->GetException());
	Close();
}

/**
 * Processes data for the stream.
 */
void TlsStream::HandleIO(void)
{
	char data[16 * 1024];
	int rc;

	if (!IsConnected()) {
		rc = SSL_do_handshake(m_SSL.get());

		if (rc == 1) {
			SetConnected(true);
		} else {
			switch (SSL_get_error(m_SSL.get(), rc)) {
				case SSL_ERROR_WANT_WRITE:
					/* fall through */
				case SSL_ERROR_WANT_READ:
					return;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return;
				default:
					I2Stream_check_exception(m_BIO);
					throw_exception(OpenSSLException("SSL_do_handshake failed", ERR_get_error()));
			}
		}
	}

	bool new_data = false, read_ok = true;

	while (read_ok) {
		rc = SSL_read(m_SSL.get(), data, sizeof(data));

		if (rc > 0) {
			m_RecvQueue->Write(data, rc);
			new_data = true;
		} else {
			switch (SSL_get_error(m_SSL.get(), rc)) {
				case SSL_ERROR_WANT_WRITE:
					/* fall through */
				case SSL_ERROR_WANT_READ:
					read_ok = false;
					break;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return;
				default:
					I2Stream_check_exception(m_BIO);
					throw_exception(OpenSSLException("SSL_read failed", ERR_get_error()));
			}
		}
	}

	if (new_data)
		OnDataAvailable(GetSelf());

	while (m_SendQueue->GetAvailableBytes() > 0) {
		size_t count = m_SendQueue->GetAvailableBytes();

		if (count == 0)
			break;

		if (count > sizeof(data))
			count = sizeof(data);

		m_SendQueue->Peek(data, count);

		rc = SSL_write(m_SSL.get(), (const char *)data, count);

		if (rc > 0) {
			m_SendQueue->Read(NULL, rc);
		} else {
			switch (SSL_get_error(m_SSL.get(), rc)) {
				case SSL_ERROR_WANT_READ:
					/* fall through */
				case SSL_ERROR_WANT_WRITE:
					return;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return;
				default:
					I2Stream_check_exception(m_BIO);
					throw_exception(OpenSSLException("SSL_write failed", ERR_get_error()));
			}
		}
	}
}

/**
 * Closes the stream.
 */
void TlsStream::Close(void)
{
	if (m_SSL)
		SSL_shutdown(m_SSL.get());

	m_SendQueue->Close();
	m_RecvQueue->Close();

	Stream::Close();
}

size_t TlsStream::GetAvailableBytes(void) const
{
	return m_RecvQueue->GetAvailableBytes();
}

size_t TlsStream::Peek(void *buffer, size_t count)
{
	return m_RecvQueue->Peek(buffer, count);
}

size_t TlsStream::Read(void *buffer, size_t count)
{
	return m_RecvQueue->Read(buffer, count);
}

void TlsStream::Write(const void *buffer, size_t count)
{
	m_SendQueue->Write(buffer, count);

	HandleIO();
}
