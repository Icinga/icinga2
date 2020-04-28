/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/tlsstream.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/configuration.hpp"
#include "base/convert.hpp"
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <ios>
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>
#include <sstream>

using namespace icinga;

bool UnbufferedAsioTlsStream::IsVerifyOK() const
{
	return m_VerifyOK;
}

String UnbufferedAsioTlsStream::GetVerifyError() const
{
	return m_VerifyError;
}

std::shared_ptr<X509> UnbufferedAsioTlsStream::GetPeerCertificate()
{
	return std::shared_ptr<X509>(SSL_get_peer_certificate(native_handle()), X509_free);
}

void UnbufferedAsioTlsStream::BeforeHandshake(handshake_type type)
{
	namespace ssl = boost::asio::ssl;

	set_verify_mode(ssl::verify_peer | ssl::verify_client_once);

	set_verify_callback([this](bool preverified, ssl::verify_context& ctx) {
		if (!preverified) {
			m_VerifyOK = false;

			std::ostringstream msgbuf;
			int err = X509_STORE_CTX_get_error(ctx.native_handle());

			msgbuf << "code " << err << ": " << X509_verify_cert_error_string(err);
			m_VerifyError = msgbuf.str();
		}

		return true;
	});

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
	if (type == client && !m_Hostname.IsEmpty()) {
		String environmentName = Application::GetAppEnvironment();
		String serverName = m_Hostname;

		if (!environmentName.IsEmpty())
			serverName += ":" + environmentName;

		SSL_set_tlsext_host_name(native_handle(), serverName.CStr());
	}
#endif /* SSL_CTRL_SET_TLSEXT_HOSTNAME */
}

std::streamsize OneCharOrBlockSource::read(char *buf, std::streamsize bufSize)
{
	namespace ios = boost::iostreams;
	using ct = ios::char_traits<char>;

	if (bufSize < 1u) {
		return ios::WOULD_BLOCK;
	}

	if (ct::is_good(m_Char)) {
		*buf = ct::to_char_type(m_Char);
		m_Char = ios::WOULD_BLOCK;
		return 1u;
	} else {
		return m_Char;
	}
}
