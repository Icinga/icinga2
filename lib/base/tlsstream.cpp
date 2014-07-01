/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/tlsstream.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/logger_fwd.hpp"
#include <boost/bind.hpp>
#include <iostream>

using namespace icinga;

int I2_EXPORT TlsStream::m_SSLIndex;
bool I2_EXPORT TlsStream::m_SSLIndexInitialized = false;

/**
 * Constructor for the TlsStream class.
 *
 * @param role The role of the client.
 * @param sslContext The SSL context for the client.
 */
TlsStream::TlsStream(const Socket::Ptr& socket, ConnectionRole role, shared_ptr<SSL_CTX> sslContext)
	: m_Socket(socket), m_Role(role)
{
	m_SSL = shared_ptr<SSL>(SSL_new(sslContext.get()), SSL_free);

	if (!m_SSL) {
		std::ostringstream msgbuf;
		msgbuf << "SSL_new() failed with code " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), NULL) << "\"";
		Log(LogCritical, "TlsStream", msgbuf.str());

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

	socket->MakeNonBlocking();

	m_BIO = BIO_new_socket(socket->GetFD(), 0);
	BIO_set_nbio(m_BIO, 1);
	SSL_set_bio(m_SSL.get(), m_BIO, m_BIO);

	if (m_Role == RoleServer)
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
	for (;;) {
		int rc, err;

		{
			boost::mutex::scoped_lock lock(m_SSLLock);
			rc = SSL_do_handshake(m_SSL.get());

			if (rc > 0)
				break;

			err = SSL_get_error(m_SSL.get(), rc);
		}

		switch (err) {
			case SSL_ERROR_WANT_READ:
				try {
					m_Socket->Poll(true, false);
				} catch (std::exception&) {}
				continue;
			case SSL_ERROR_WANT_WRITE:
				try {
					m_Socket->Poll(false, true);
				} catch (std::exception&) {}
				continue;
			case SSL_ERROR_ZERO_RETURN:
				Close();
				return;
			default:
				std::ostringstream msgbuf;
				msgbuf << "SSL_do_handshake() failed with code " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), NULL) << "\"";
				Log(LogCritical, "TlsStream", msgbuf.str());

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
	size_t left = count;

	while (left > 0) {
		int rc, err;

		{
			boost::mutex::scoped_lock lock(m_SSLLock);
			rc = SSL_read(m_SSL.get(), ((char *)buffer) + (count - left), left);

			if (rc <= 0)
				err = SSL_get_error(m_SSL.get(), rc);
		}

		if (rc <= 0) {
			switch (err) {
				case SSL_ERROR_WANT_READ:
					try {
						m_Socket->Poll(true, false);
					} catch (std::exception&) {}
					continue;
				case SSL_ERROR_WANT_WRITE:
					try {
						m_Socket->Poll(false, true);
					} catch (std::exception&) {}
					continue;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return count - left;
				default:
					std::ostringstream msgbuf;
					msgbuf << "SSL_read() failed with code " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), NULL) << "\"";
					Log(LogCritical, "TlsStream", msgbuf.str());

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
	size_t left = count;

	while (left > 0) {
		int rc, err;

		{
			boost::mutex::scoped_lock lock(m_SSLLock);
			rc = SSL_write(m_SSL.get(), ((const char *)buffer) + (count - left), left);

			if (rc <= 0)
				err = SSL_get_error(m_SSL.get(), rc);
		}

		if (rc <= 0) {
			switch (err) {
				case SSL_ERROR_WANT_READ:
					try {
						m_Socket->Poll(true, false);
					} catch (std::exception&) {}
					continue;
				case SSL_ERROR_WANT_WRITE:
					try {
						m_Socket->Poll(false, true);
					} catch (std::exception&) {}
					continue;
				case SSL_ERROR_ZERO_RETURN:
					Close();
					return;
				default:
					std::ostringstream msgbuf;
					msgbuf << "SSL_write() failed with code " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), NULL) << "\"";
					Log(LogCritical, "TlsStream", msgbuf.str());

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
	for (;;) {
		int rc, err;

		{
			boost::mutex::scoped_lock lock(m_SSLLock);

			do {
				rc = SSL_shutdown(m_SSL.get());
			} while (rc == 0);

			if (rc > 0)
				break;

			err = SSL_get_error(m_SSL.get(), rc);
		}

		switch (err) {
			case SSL_ERROR_WANT_READ:
				try {
					m_Socket->Poll(true, false);
				} catch (std::exception&) {}
				continue;
			case SSL_ERROR_WANT_WRITE:
				try {
					m_Socket->Poll(false, true);
				} catch (std::exception&) {}
				continue;
			default:
				goto close_socket;
		}
	}

close_socket:
	m_Socket->Close();
}

bool TlsStream::IsEof(void) const
{
	return (BIO_eof(m_BIO) == 1);
}
