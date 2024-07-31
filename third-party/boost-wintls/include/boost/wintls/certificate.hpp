//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_CERTIFICATE_HPP
#define BOOST_WINTLS_CERTIFICATE_HPP

#include <boost/wintls/error.hpp>
#include <boost/wintls/file_format.hpp>

#include <boost/wintls/detail/win32_crypto.hpp>

#include <memory>

namespace boost {
namespace wintls {

namespace detail {
struct crypt_context {
  crypt_context(const std::string& name) {
    if (!CryptAcquireContextA(&ptr, name.c_str(), nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_SILENT)) {
      detail::throw_last_error("CryptAcquireContextA");
    }
  }

  ~crypt_context() {
    CryptReleaseContext(ptr, 0);
  }

  HCRYPTPROV ptr = 0;
};

struct crypt_key {
  ~crypt_key() {
    CryptDestroyKey(ptr);
  }

  HCRYPTKEY ptr = 0;
};

struct cert_ctx_deleter {
  void operator()(const CERT_CONTEXT* cert_ctx) {
    CertFreeCertificateContext(cert_ctx);
  }
};

}
/**
 * @verbatim embed:rst:leading-asterisk
 * Custom std::unique_ptr for managing a `CERT_CONTEXT`_
 * @endverbatim
 */
using cert_context_ptr = std::unique_ptr<const CERT_CONTEXT, detail::cert_ctx_deleter>;

/**
 * @verbatim embed:rst:leading-asterisk
 * Convert certificate from standard X509 format to Windows `CERT_CONTEXT`_.
 * @endverbatim
 *
 * @param x509 Buffer holding the X509 certificate contents.
 *
 * @param format The @ref file_format of the X509 contents.
 *
 * @return A managed cert_context.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 */
inline cert_context_ptr x509_to_cert_context(const net::const_buffer& x509, file_format format) {
  // TODO: Support DER format
  BOOST_VERIFY_MSG(format == file_format::pem, "Only PEM format currently implemented");

  auto data = detail::crypt_string_to_binary(x509);
  auto cert = CertCreateCertificateContext(X509_ASN_ENCODING, data.data(), static_cast<DWORD>(data.size()));
  if (!cert) {
    detail::throw_last_error("CertCreateCertificateContext");
  }

  return cert_context_ptr{cert};
}

/**
 * @verbatim embed:rst:leading-asterisk
 * Convert certificate from standard X509 format to Windows `CERT_CONTEXT`_.
 * @endverbatim
 *
 * @param x509 Buffer holding the X509 certificate contents.
 *
 * @param format The @ref file_format of the X509 contents.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @return A managed cert_context.
 *
 */
inline cert_context_ptr x509_to_cert_context(const net::const_buffer& x509, file_format format, boost::system::error_code& ec) {
  ec = {};
  try {
    return x509_to_cert_context(x509, format);
  } catch (const boost::system::system_error& e) {
    ec = e.code();
    return cert_context_ptr{};
  }
}

/**
 * Import a private key into the default cryptographic provider using the given name.
 *
 * This function can be used to import an RSA private key in PKCS#8
 * format in to the default certificate provider under the given name.
 *
 * The key can be associated with a certificate using the @ref assign_private_key function.
 *
 * @param private_key The private key to be imported in PKCS#8 format.
 *
 * @param format The @ref file_format of the private_key.
 *
 * @param name The name used to associate the key.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note Currently only RSA keys are supported.
 */
inline void import_private_key(const net::const_buffer& private_key, file_format format, const std::string& name) {
  // TODO: Handle ASN.1 DER format
  BOOST_VERIFY_MSG(format == file_format::pem, "Only PEM format currently implemented");
  auto data = detail::crypt_decode_object_ex(net::buffer(detail::crypt_string_to_binary(private_key)), PKCS_PRIVATE_KEY_INFO);
  auto private_key_info = reinterpret_cast<CRYPT_PRIVATE_KEY_INFO*>(data.data());

  // TODO: Set proper error code instead of asserting
  BOOST_VERIFY_MSG(strcmp(private_key_info->Algorithm.pszObjId, szOID_RSA_RSA) == 0, "Only RSA keys supported");
  auto rsa_private_key = detail::crypt_decode_object_ex(net::buffer(private_key_info->PrivateKey.pbData,
                                                                    private_key_info->PrivateKey.cbData),
                                                        PKCS_RSA_PRIVATE_KEY);

  detail::crypt_context ctx(name);
  detail::crypt_key key;
  if (!CryptImportKey(ctx.ptr,
                      rsa_private_key.data(),
                      static_cast<DWORD>(rsa_private_key.size()),
                      0,
                      0,
                      &key.ptr)) {
    detail::throw_last_error("CryptImportKey");
  }
}

/**
 * Import a private key into the default cryptographic provider using the given name.
 *
 * This function can be used to import an RSA private key in PKCS#8
 * format in to the default certificate provider under the given name.
 *
 * The key can be associated with a certificate using the @ref assign_private_key function.
 *
 * @param private_key The private key to be imported in PKCS#8 format.
 *
 * @param format The @ref file_format of the private_key.
 *
 * @param name The name used to associate the key.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @note Currently only RSA keys are supported.
 */
inline void import_private_key(const net::const_buffer& private_key, file_format format, const std::string& name, boost::system::error_code& ec) {
  ec = {};
  try {
    import_private_key(private_key, format, name);
  } catch (const boost::system::system_error& e) {
    ec = e.code();
  }
}

/**
 * Delete a private key from the default cryptographic provider.
 *
 * @param name The name of the container storing the private key to delete.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 */
inline void delete_private_key(const std::string& name) {
  HCRYPTKEY ptr = 0;
  if (!CryptAcquireContextA(&ptr, name.c_str(), nullptr, PROV_RSA_FULL, CRYPT_DELETEKEYSET)) {

    throw boost::system::system_error(static_cast<int>(GetLastError()), boost::system::system_category());
  }
}

/**
 * Delete a private key from the default cryptographic provider.
 *
 * @param name The name of the container storing the private key to delete.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 */
inline void delete_private_key(const std::string& name, boost::system::error_code& ec) {
  ec = {};
  try {
    delete_private_key(name);
  } catch (const boost::system::system_error& e) {
    ec = e.code();
  }
}

/**
 * @verbatim embed:rst:leading-asterisk
 * Assigns a private key to a certificate.
 *
 * In order for a `CERT_CONTEXT`_ to be used by a server in needs to
 * have a private key associated with it.
 *
 * @endverbatim
 *
 * This function will associate the named key with the given
 * certficate in order for it be used by eg. @ref context::use_certificate.
 *
 * @note No check is done to ensure the key exists. Associating a non
 * existing or non accessible key will result in unexpected behavior
 * when used with a @ref stream operating as a server.
 *
 * @param cert The certificate to associate with the private key.
 *
 * @param name The name of the private key in the default cryptographic key provider.
 *
 * @throws boost::system::system_error Thrown on failure.
 */
inline void assign_private_key(const CERT_CONTEXT* cert, const std::string& name) {
  // TODO: Move to utility function
  const auto size = name.size() + 1;
  auto wname = std::make_unique<WCHAR[]>(size);
  const auto size_converted = mbstowcs(wname.get(), name.c_str(), size);
  BOOST_VERIFY_MSG(size_converted == name.size(), "mbstowcs");

  CRYPT_KEY_PROV_INFO keyProvInfo{};
  keyProvInfo.pwszContainerName = wname.get();
  keyProvInfo.pwszProvName = nullptr;
  keyProvInfo.dwFlags = CERT_SET_KEY_PROV_HANDLE_PROP_ID | CERT_SET_KEY_CONTEXT_PROP_ID;
  keyProvInfo.dwProvType = PROV_RSA_FULL;
  keyProvInfo.dwKeySpec = AT_KEYEXCHANGE;

  if (!CertSetCertificateContextProperty(cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo)) {
    detail::throw_last_error("CertSetCertificateContextProperty");
  }
}

/**
 * @verbatim embed:rst:leading-asterisk
 * Assigns a private key to a certificate.
 *
 * In order for a `CERT_CONTEXT`_ to be used by a server in needs to
 * have a private key associated with it.
 *
 * @endverbatim
 *
 * This function will associate the named key with the given
 * certficate in order for it be used by eg. @ref context::use_certificate.
 *
 * @note No check is done to ensure the key exists. Associating a non
 * existing or non accessible key will result in unexpected behavior
 * when used with a @ref stream operating as a server.
 *
 * @param cert The certificate to associate with the private key.
 *
 * @param name The name of the private key in the default cryptographic key provider.
 *
 * @param ec Set to indicate what error occurred, if any.
 */
inline void assign_private_key(const CERT_CONTEXT* cert, const std::string& name, boost::system::error_code& ec) {
  ec = {};
  try {
    assign_private_key(cert, name);
  } catch (const boost::system::system_error& e) {
    ec = e.code();
  }
}

} // namespace wintls
} // namespace boost

#endif
