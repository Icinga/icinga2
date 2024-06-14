//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_CONTEXT_HPP
#define BOOST_WINTLS_CONTEXT_HPP

#include <boost/wintls/method.hpp>

#include <boost/wintls/detail/config.hpp>
#include <boost/wintls/detail/context_certificates.hpp>

#include <string>

namespace boost {
namespace wintls {

namespace detail {
class sspi_handshake;
}

class context {
public:
  /** Construct a context.
   *
   * @param connection_method The @ref method to use for connections.
   */
  explicit context(method connection_method)
    : method_(connection_method)
    , verify_server_certificate_(false) {
  }

  /** Add certification authority for performing verification.
   *
   * This function is used to add one trusted certification authority
   * to the contexts certificate store used for certificate validation
   *
   * @param cert The certficate to add to the certificate store
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void add_certificate_authority(const CERT_CONTEXT* cert) {
    ctx_certs_.add_certificate_authority(cert);
  }

  /** Add certification authority for performing verification.
   *
   * This function is used to add one trusted certification authority
   * to the contexts certificate store used for certificate validation
   *
   * @param cert The certficate to add to the certificate store
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  void add_certificate_authority(const CERT_CONTEXT* cert, boost::system::error_code& ec) {
    try {
      ctx_certs_.add_certificate_authority(cert);
    } catch (const boost::system::system_error& e) {
      ec = e.code();
    }
  }

  /** Enables/disables remote server certificate verification
   *
   * This function may be used to enable clients to verify the
   * certificate presented by the server with the known trusted
   * certificates.
   *
   * @param verify True if the remote server certificate should be
   * verified
   */
  void verify_server_certificate(bool verify) {
    verify_server_certificate_ = verify;
  }

  /** Use the default operating system certificates
   *
   * This function may be used to verify the server certficates
   * against the certficates installed in the operating system when
   * performing handshakes as a client.
   *
   * It is still possible to add additional certificates for
   * verification in addition to the ones installed by the operating
   * system.
   *
   * @param use_system_certs True if the default operating system
   * certificates should be used for verification.
   */
  void use_default_certificates(bool use_system_certs) {
    ctx_certs_.use_default_cert_store = use_system_certs;
  }

  /** Set the certificate to use when operating as a server
   *
   * This function sets the certficate to use when using a @ref stream
   * as server.
   *
   * @param cert The private certificate the @ref stream will use for
   * encrypting messages when operating as a server.
   *
   * @note The certificate must be a private certificate,
   * ie. containing a reference to a private key. If the certificate
   * is imported from a public certificate @ref assign_private_key can
   * be used for that.
   *
   * @throws boost::system::system_error Thrown on failure.
   */
  void use_certificate(const CERT_CONTEXT* cert) {
    ctx_certs_.use_certificate(cert);
  }

  /** Set the certificate to use when operating as a server
   *
   * This function sets the certficate to use when using a @ref stream
   * as server.
   *
   * @param cert The private certificate the @ref stream will use for
   * encrypting messages when operating as a server.
   *
   * @note The certificate must be a private certificate,
   * ie. containing a reference to a private key. If the certificate
   * is imported from a public certificate @ref assign_private_key can
   * be used for that.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  void use_certificate(const CERT_CONTEXT* cert, boost::system::error_code& ec) {
    try {
      ctx_certs_.use_certificate(cert);
    } catch (const boost::system::system_error& e) {
      ec = e.code();
    }
  }

private:
  DWORD verify_certificate(const CERT_CONTEXT* cert, const std::string& server_hostname, bool check_revocation) {
    if (!verify_server_certificate_) {
      return ERROR_SUCCESS;
    }
    return static_cast<DWORD>(ctx_certs_.verify_certificate(cert, server_hostname, check_revocation));
  }

  const CERT_CONTEXT* server_cert() const {
    return ctx_certs_.server_cert();
  }

  friend class detail::sspi_handshake;

  detail::context_certificates ctx_certs_;
  method method_;
  bool verify_server_certificate_;
};

} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_CONTEXT_HPP
