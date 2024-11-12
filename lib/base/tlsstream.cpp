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
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>
#include <sstream>

using namespace icinga;

/**
 * Checks whether the TLS handshake was completed with a valid peer certificate.
 *
 * @return true if the peer presented a valid certificate, false otherwise
 */
bool UnbufferedAsioTlsStream::IsVerifyOK()
{
	if (!SSL_is_init_finished(native_handle())) {
		// handshake was not completed
		return false;
	}

	if (GetPeerCertificate() == nullptr) {
		// no peer certificate was sent
		return false;
	}

	return SSL_get_verify_result(native_handle()) == X509_V_OK;
}

/**
 * Returns a human-readable error string for situations where IsVerifyOK() returns false.
 *
 * If the handshake was completed and a peer certificate was provided,
 * the string additionally contains the OpenSSL verification error code.
 *
 * @return string containing the error message
 */
String UnbufferedAsioTlsStream::GetVerifyError()
{
	if (!SSL_is_init_finished(native_handle())) {
		return "handshake not completed";
	}

	if (GetPeerCertificate() == nullptr) {
		return "no peer certificate provided";
	}

	std::ostringstream buf;
	long err = SSL_get_verify_result(native_handle());
	buf << "code " << err << ": " << X509_verify_cert_error_string(err);
	return buf.str();
}

std::shared_ptr<X509> UnbufferedAsioTlsStream::GetPeerCertificate()
{
	return std::shared_ptr<X509>(SSL_get_peer_certificate(native_handle()), X509_free);
}

void UnbufferedAsioTlsStream::BeforeHandshake(handshake_type type)
{
	namespace ssl = boost::asio::ssl;

	if (!m_Hostname.IsEmpty()) {
		X509_VERIFY_PARAM_set1_host(SSL_get0_param(native_handle()), m_Hostname.CStr(), m_Hostname.GetLength());
	}

	set_verify_mode(ssl::verify_peer | ssl::verify_client_once);

	set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
		(void) preverified;
		(void) ctx;

		/* Continue the handshake even if an invalid peer certificate was presented. The verification result has to be
		 * checked using the IsVerifyOK() method.
		 *
		 * Such connections are used for the initial enrollment of nodes where they use a self-signed certificate to
		 * send a certificate request and receive their valid certificate after approval (manually by the administrator
		 * or using a certificate ticket).
		 */
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
