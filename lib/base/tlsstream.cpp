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

#include "base/tlsstream.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
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
TlsStream::TlsStream(const Socket::Ptr& socket, ConnectionRole role, const boost::shared_ptr<SSL_CTX>& sslContext)
	: m_Eof(false), m_VerifyOK(true), m_Socket(socket), m_Role(role)
{
	std::ostringstream msgbuf;
	char errbuf[120];

	m_SSL = boost::shared_ptr<SSL>(SSL_new(sslContext.get()), SSL_free);

	if (!m_SSL) {
		msgbuf << "SSL_new() failed with code " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		Log(LogCritical, "TlsStream", msgbuf.str());

		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SSL_new")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!m_SSLIndexInitialized) {
		m_SSLIndex = SSL_get_ex_new_index(0, const_cast<char *>("TlsStream"), NULL, NULL, NULL);
		m_SSLIndexInitialized = true;
	}

	SSL_set_ex_data(m_SSL.get(), m_SSLIndex, this);

	SSL_set_verify(m_SSL.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &TlsStream::ValidateCertificate);

	socket->MakeNonBlocking();

	SSL_set_fd(m_SSL.get(), socket->GetFD());

	if (m_Role == RoleServer)
		SSL_set_accept_state(m_SSL.get());
	else
		SSL_set_connect_state(m_SSL.get());
}

int TlsStream::ValidateCertificate(int preverify_ok, X509_STORE_CTX *ctx)
{
	SSL *ssl = static_cast<SSL *>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
	TlsStream *stream = static_cast<TlsStream *>(SSL_get_ex_data(ssl, m_SSLIndex));
	if (!preverify_ok)
		stream->m_VerifyOK = false;
	return 1;
}

bool TlsStream::IsVerifyOK(void) const
{
	return m_VerifyOK;
}

/**
 * Retrieves the X509 certficate for this client.
 *
 * @returns The X509 certificate.
 */
boost::shared_ptr<X509> TlsStream::GetClientCertificate(void) const
{
	boost::mutex::scoped_lock lock(m_SSLLock);
	return boost::shared_ptr<X509>(SSL_get_certificate(m_SSL.get()), &Utility::NullDeleter);
}

/**
 * Retrieves the X509 certficate for the peer.
 *
 * @returns The X509 certificate.
 */
boost::shared_ptr<X509> TlsStream::GetPeerCertificate(void) const
{
	boost::mutex::scoped_lock lock(m_SSLLock);
	return boost::shared_ptr<X509>(SSL_get_peer_certificate(m_SSL.get()), X509_free);
}

void TlsStream::Handshake(void)
{
	std::ostringstream msgbuf;
	char errbuf[120];

	boost::mutex::scoped_lock alock(m_IOActionLock);

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
				} catch (const std::exception&) {}
				continue;
			case SSL_ERROR_WANT_WRITE:
				try {
					m_Socket->Poll(false, true);
				} catch (const std::exception&) {}
				continue;
			case SSL_ERROR_ZERO_RETURN:
				CloseUnlocked();
				return;
			default:
				msgbuf << "SSL_do_handshake() failed with code " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
				Log(LogCritical, "TlsStream", msgbuf.str());

				BOOST_THROW_EXCEPTION(openssl_error()
				    << boost::errinfo_api_function("SSL_do_handshake")
				    << errinfo_openssl_error(ERR_peek_error()));
		}
	}
}

/**
 * Processes data for the stream.
 */
size_t TlsStream::Read(void *buffer, size_t count)
{
	size_t left = count;
	std::ostringstream msgbuf;
	char errbuf[120];

	bool want_read;

	{
		boost::mutex::scoped_lock lock(m_SSLLock);
		want_read = !SSL_pending(m_SSL.get()) || SSL_want_read(m_SSL.get());
	}

	if (want_read)
		m_Socket->Poll(true, false);

	boost::mutex::scoped_lock alock(m_IOActionLock);

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
					} catch (const std::exception&) {}
					continue;
				case SSL_ERROR_WANT_WRITE:
					try {
						m_Socket->Poll(false, true);
					} catch (const std::exception&) {}
					continue;
				case SSL_ERROR_ZERO_RETURN:
					CloseUnlocked();
					return count - left;
				default:
					if (ERR_peek_error() != 0) {
						msgbuf << "SSL_read() failed with code " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
						Log(LogCritical, "TlsStream", msgbuf.str());
					}

					BOOST_THROW_EXCEPTION(openssl_error()
					    << boost::errinfo_api_function("SSL_read")
					    << errinfo_openssl_error(ERR_peek_error()));
			}
		}

		left -= rc;
	}

	return count;
}

void TlsStream::Write(const void *buffer, size_t count)
{
	size_t left = count;
	std::ostringstream msgbuf;
	char errbuf[120];

	m_Socket->Poll(false, true);

	boost::mutex::scoped_lock alock(m_IOActionLock);

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
					} catch (const std::exception&) {}
					continue;
				case SSL_ERROR_WANT_WRITE:
					try {
						m_Socket->Poll(false, true);
					} catch (const std::exception&) {}
					continue;
				case SSL_ERROR_ZERO_RETURN:
					CloseUnlocked();
					return;
				default:
					if (ERR_peek_error() != 0) {
						msgbuf << "SSL_write() failed with code " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
						Log(LogCritical, "TlsStream", msgbuf.str());
					}

					BOOST_THROW_EXCEPTION(openssl_error()
					    << boost::errinfo_api_function("SSL_write")
					    << errinfo_openssl_error(ERR_peek_error()));
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
	boost::mutex::scoped_lock alock(m_IOActionLock);

	CloseUnlocked();
}

void TlsStream::CloseUnlocked(void)
{
	m_Eof = true;

	for (int i = 0; i < 5; i++) {
		int rc, err;

		{
			boost::mutex::scoped_lock lock(m_SSLLock);
			rc = SSL_shutdown(m_SSL.get());

			if (rc == 0)
				continue;

			if (rc > 0)
				break;

			err = SSL_get_error(m_SSL.get(), rc);
		}

		switch (err) {
			case SSL_ERROR_WANT_READ:
				try {
					m_Socket->Poll(true, false);
				} catch (const std::exception&) {}
				continue;
			case SSL_ERROR_WANT_WRITE:
				try {
					m_Socket->Poll(false, true);
				} catch (const std::exception&) {}
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
	return m_Eof;
}
