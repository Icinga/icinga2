/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/tlsstream.h"
#include "base/stream_bio.h"
#include "base/objectlock.h"
#include "base/debug.h"
#include "base/utility.h"
#include "base/exception.h"
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

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
	: m_SSLContext(sslContext), m_Role(role)
{
	m_InnerStream = dynamic_pointer_cast<BufferedStream>(innerStream);
	
	if (!m_InnerStream)
		m_InnerStream = boost::make_shared<BufferedStream>(innerStream);

	m_InnerStream->MakeNonBlocking();
	
	m_SSL = shared_ptr<SSL>(SSL_new(m_SSLContext.get()), SSL_free);

	m_SSLContext.reset();

	if (!m_SSL) {
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SSL_new")
			<< errinfo_openssl_error(ERR_get_error()));
	}

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

void TlsStream::Handshake(void)
{
	ASSERT(!OwnsLock());

	int rc;

	ObjectLock olock(this);

	while ((rc = SSL_do_handshake(m_SSL.get())) <= 0) {
		switch (SSL_get_error(m_SSL.get(), rc)) {
			case SSL_ERROR_WANT_READ:
				olock.Unlock();
				m_InnerStream->WaitReadable(1);
				olock.Lock();
				continue;
			case SSL_ERROR_WANT_WRITE:
				olock.Unlock();
				m_InnerStream->WaitWritable(1);
				olock.Lock();
				continue;
			case SSL_ERROR_ZERO_RETURN:
				Close();
				return;
			default:
				I2Stream_check_exception(m_BIO);
				BOOST_THROW_EXCEPTION(openssl_error()
				    << boost::errinfo_api_function("SSL_do_handshake")
				    << errinfo_openssl_error(ERR_get_error()));
		}
	}
}

/**
 * Processes data for the stream.
 */
size_t TlsStream::Read(void *buffer, size_t count)
{
	ASSERT(!OwnsLock());

	size_t left = count;

	ObjectLock olock(this);

	while (left > 0) {
		int rc = SSL_read(m_SSL.get(), ((char *)buffer) + (count - left), left);

		if (rc <= 0) {
			switch (SSL_get_error(m_SSL.get(), rc)) {
				case SSL_ERROR_WANT_READ:
					olock.Unlock();
					m_InnerStream->WaitReadable(1);
					olock.Lock();
					continue;
				case SSL_ERROR_WANT_WRITE:
					olock.Unlock();
					m_InnerStream->WaitWritable(1);
					olock.Lock();
					continue;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return count - left;
				default:
					I2Stream_check_exception(m_BIO);
					BOOST_THROW_EXCEPTION(openssl_error()
					    << boost::errinfo_api_function("SSL_read")
					    << errinfo_openssl_error(ERR_get_error()));
			}
		}

		left -= rc;
	}

	return count;
}

void TlsStream::Write(const void *buffer, size_t count)
{
	ASSERT(!OwnsLock());

	size_t left = count;

	ObjectLock olock(this);

	while (left > 0) {
		int rc = SSL_write(m_SSL.get(), ((const char *)buffer) + (count - left), left);

		if (rc <= 0) {
			switch (SSL_get_error(m_SSL.get(), rc)) {
				case SSL_ERROR_WANT_READ:
					olock.Unlock();
					m_InnerStream->WaitReadable(1);
					olock.Lock();
					continue;
				case SSL_ERROR_WANT_WRITE:
					olock.Unlock();
					m_InnerStream->WaitWritable(1);
					olock.Lock();
					continue;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return;
				default:
					I2Stream_check_exception(m_BIO);
					BOOST_THROW_EXCEPTION(openssl_error()
					    << boost::errinfo_api_function("SSL_write")
					    << errinfo_openssl_error(ERR_get_error()));
			}
		}

		left -= rc;
	}
}

/**
 * Closes the stream.
 */
void TlsStream::Close(void)
{
	m_InnerStream->Close();
}

bool TlsStream::IsEof(void) const
{
	return m_InnerStream->IsEof();
}
