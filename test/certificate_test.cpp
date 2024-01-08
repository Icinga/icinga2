//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "certificate.hpp"
#include "ocsp_responder.hpp"
#include "unittest.hpp"

#include <boost/wintls/certificate.hpp>
#include <boost/wintls/error.hpp>
#include <boost/wintls/detail/context_certificates.hpp>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace {
std::string get_cert_name(const CERT_CONTEXT* cert) {
  auto size = CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, nullptr, 0);
  REQUIRE(size > 0);
  std::vector<char> str(size);
  CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, str.data(), size);
  return {str.data()};
}

bool container_exists(const std::string& name) {
  HCRYPTKEY ptr = 0;
  if (!CryptAcquireContextA(&ptr, name.c_str(), nullptr, PROV_RSA_FULL, CRYPT_SILENT)) {
    auto last_error = GetLastError();
    if (last_error == static_cast<DWORD>(NTE_BAD_KEYSET)) {
      return false;
    }
    throw boost::system::system_error(static_cast<int>(last_error), boost::system::system_category());
  }
  CryptReleaseContext(ptr, 0);
  return true;
}

// Add a certificate from the PEM formatted string cert_str to the given store.
// If get_handle is true, a handle to the certificate in the store is returned.
// Otherwise, the certificate will be owned by the store and nullptr is returned.
boost::wintls::cert_context_ptr add_cert_str_to_store(HCERTSTORE store,
                                                      const net::const_buffer& cert_str,
                                                      bool get_handle) {
  PCCERT_CONTEXT cert_in_store = nullptr;
  const auto cert_data = boost::wintls::detail::crypt_string_to_binary(cert_str);
  if (!CertAddEncodedCertificateToStore(store,
                                        X509_ASN_ENCODING,
                                        cert_data.data(),
                                        static_cast<DWORD>(cert_data.size()),
                                        CERT_STORE_ADD_ALWAYS,
                                        get_handle ? &cert_in_store : nullptr)) {
    boost::wintls::detail::throw_last_error("CertAddEncodedCertificateToStore");
  }
  if (get_handle) {
    return boost::wintls::cert_context_ptr{cert_in_store};
  }
  return boost::wintls::cert_context_ptr{};
}

// Load a PEM formatted certificate chain from the given file.
// Assumes that the first certificate in the file is the leaf certificate
// and returns a context for this certificate which internally holds a store
// containing all remaining certificates from the file.
boost::wintls::cert_context_ptr load_chain_file(const std::string& path) {
  std::ifstream ifs(path);
  if (ifs.fail()) {
    throw std::runtime_error("Failed to open file " + path);
  }
  constexpr auto begin_cert_str = "-----BEGIN CERTIFICATE-----";
  constexpr auto end_cert_str = "-----END CERTIFICATE-----";
  constexpr std::size_t end_cert_str_size = 25;
  // find the first certificate in the file
  std::string str{std::istreambuf_iterator<char>{ifs}, {}};
  auto cert_begin = str.find(begin_cert_str, 0);
  auto cert_end = str.find(end_cert_str, cert_begin);
  if (cert_begin == std::string::npos || cert_end == std::string::npos) {
    throw std::runtime_error("Error parsing certificate chain in PEM format from file " + path);
  }
  // Open a temporary store for the chain.
  // Use CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, so the context of the leaf certificate can take ownership.
  const auto chain_store = boost::wintls::detail::cert_store_ptr{
      CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, nullptr)};
  auto leaf_ctx = add_cert_str_to_store(chain_store.get(),
                                        net::buffer(&str[cert_begin], cert_end + end_cert_str_size - cert_begin),
                                        true);
  // add any remaining certificates to the chain_store
  for (cert_begin = str.find(begin_cert_str, cert_end);
       cert_begin != std::string::npos;
       cert_begin = str.find(begin_cert_str, cert_end)) {
    cert_end = str.find(end_cert_str, cert_begin);
    if (cert_end == std::string::npos) {
      throw std::runtime_error("Error parsing certificate chain in PEM format from file " + path);
    }
    add_cert_str_to_store(chain_store.get(),
                          net::buffer(&str[cert_begin], cert_end + end_cert_str_size - cert_begin),
                          false);
  }
  return leaf_ctx;
}

struct crl_ctx_deleter {
  void operator()(const CRL_CONTEXT* crl_ctx) {
    CertFreeCRLContext(crl_ctx);
  }
};

using crl_ctx_ptr = std::unique_ptr<const CRL_CONTEXT, crl_ctx_deleter>;

crl_ctx_ptr x509_to_crl_context(const net::const_buffer& x509) {
  const auto data = boost::wintls::detail::crypt_string_to_binary(x509);
  const auto crl = CertCreateCRLContext(X509_ASN_ENCODING, data.data(), static_cast<DWORD>(data.size()));
  if (!crl) {
    boost::wintls::detail::throw_last_error("CertCreateCRLContext");
  }
  return crl_ctx_ptr{crl};
}

crl_ctx_ptr crl_from_file(const std::string& path) {
  const auto crl_bytes = bytes_from_file(path);
  return x509_to_crl_context(net::buffer(crl_bytes));
}

boost::wintls::cert_context_ptr cert_from_file(const std::string& path) {
  const auto cert_bytes = bytes_from_file(path);
  return boost::wintls::x509_to_cert_context(net::buffer(cert_bytes), boost::wintls::file_format::pem);
}
}

TEST_CASE("certificate conversion") {
  SECTION("valid cert bytes") {
    const auto cert = boost::wintls::x509_to_cert_context(net::buffer(test_certificate), boost::wintls::file_format::pem);
    CHECK(get_cert_name(cert.get()) == "localhost");
  }

  SECTION("invalid cert bytes") {
    const std::vector<char> cert_bytes;
    CHECK_THROWS(boost::wintls::x509_to_cert_context(net::buffer(cert_bytes), boost::wintls::file_format::pem));
    auto error = boost::system::errc::make_error_code(boost::system::errc::success);
    const auto cert = boost::wintls::x509_to_cert_context(net::buffer(cert_bytes), boost::wintls::file_format::pem, error);
    CHECK(error);
    CHECK_FALSE(cert);
  }
}

TEST_CASE("import private key") {
  const std::string name{"boost::wintls crypto test container"};
  REQUIRE_FALSE(container_exists(name));

  boost::wintls::import_private_key(net::buffer(test_key), boost::wintls::file_format::pem, name);
  CHECK(container_exists(name));

  boost::system::error_code ec;
  boost::wintls::import_private_key(net::buffer(test_key), boost::wintls::file_format::pem, name, ec);
  CHECK(ec.value() == NTE_EXISTS);

  boost::wintls::delete_private_key(name);
  CHECK_FALSE(container_exists(name));

  boost::wintls::delete_private_key(name, ec);
  CHECK(ec.value() == NTE_BAD_KEYSET);
}

TEST_CASE("verify certificate host name") {
  boost::wintls::detail::context_certificates ctx_certs;
  ctx_certs.add_certificate_authority(cert_from_file(TEST_CERTIFICATES_PATH "ca_root.crt").get());

  const auto cert = load_chain_file(TEST_CERTIFICATES_PATH "leaf_chain.pem");

  // success case: host name is not verified when parameter is empty string
  CHECK(ctx_certs.verify_certificate(cert.get(), "", false) == ERROR_SUCCESS);
  // success case: certificate contains the common name "wintls.test"
  CHECK(ctx_certs.verify_certificate(cert.get(), "wintls.test", false) == ERROR_SUCCESS);
  // success case: certificate allows alternative names "*.wintls.test"
  CHECK(ctx_certs.verify_certificate(cert.get(), "subdomain.wintls.test", false) == ERROR_SUCCESS);
  // fail case: incorrect host name
  CHECK(ctx_certs.verify_certificate(cert.get(), "wrong.host", false) == CERT_E_CN_NO_MATCH);
}

TEST_CASE("check certificate revocation") {
  SECTION("single self signed certificate") {
    const auto cert = boost::wintls::x509_to_cert_context(net::buffer(test_certificate), boost::wintls::file_format::pem);
    boost::wintls::detail::context_certificates ctx_certs;
    ctx_certs.add_certificate_authority(cert.get());
    // It appears that there is no revocation check done in this case,
    // otherwise this should fail with CRYPT_E_NO_REVOCATION_CHECK
    // as the certificate does not supply any information how to check for revocation.
    // Note that this does not depend on whether we pass CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT
    // or CERT_CHAIN_REVOCATION_CHECK_CHAIN to CertGetCertificateChain.
    // So apparently root certificates are never checked for revocation, which probably makes sense.
    CHECK(ctx_certs.verify_certificate(cert.get(), "", true) == ERROR_SUCCESS);
  }

  SECTION("certificate chain with CRLs") {
    boost::wintls::detail::context_certificates ctx_certs;
    ctx_certs.add_certificate_authority(cert_from_file(TEST_CERTIFICATES_PATH "ca_root.crt").get());

    const auto cert = load_chain_file(TEST_CERTIFICATES_PATH "leaf_chain.pem");

    // fail case: cannot check for revocation (no CRL or OCSP endpoint specified and no local CRL available)
    CHECK(ctx_certs.verify_certificate(cert.get(), "", true) == CRYPT_E_NO_REVOCATION_CHECK);

    // success case: empty CRLs available
    ctx_certs.add_crl(crl_from_file(TEST_CERTIFICATES_PATH "ca_intermediate_empty.crl.pem").get());
    ctx_certs.add_crl(crl_from_file(TEST_CERTIFICATES_PATH "ca_root_empty.crl.pem").get());
    CHECK(ctx_certs.verify_certificate(cert.get(), "", true) == ERROR_SUCCESS);

    // fail case: CRL of ca_intermediate with revoked leaf certificate
    ctx_certs.add_crl(crl_from_file(TEST_CERTIFICATES_PATH "ca_intermediate_leaf_revoked.crl.pem").get());
    CHECK(ctx_certs.verify_certificate(cert.get(), "", true) == CRYPT_E_REVOKED);
  }
}

// This test takes about 15 seconds to run, as the OCSP responder takes that long to become available.
// Therefore it is marked as integration test and not run by default.
TEST_CASE("check certificate revocation (integration test)", "[.integration]") {
  SECTION("certificate chain with OCSP") {
    boost::wintls::detail::context_certificates ctx_certs;
    ctx_certs.add_certificate_authority(cert_from_file(TEST_CERTIFICATES_PATH "ca_root.crt").get());

    // fail case: OCSP responder is not running
    const auto cert_ocsp = load_chain_file(TEST_CERTIFICATES_PATH "leaf_ocsp_chain.pem");
    const auto res_without_responder = ctx_certs.verify_certificate(cert_ocsp.get(), "", true);
    // NOTE: We start the openssl OCSP responder with '-nmin 1'
    //       which causes the OCSP response to be cached by windows for one minute.
    //       This is the smallest possible value. If we do not pass -nmin or -ndays to openssl,
    //       it will seemingly consider the OCSP response to be valid indefinitely.
    //       Therefore we allow a successful response here and print a warning in that case.
    if (res_without_responder == ERROR_SUCCESS) {
      WARN("Validation of the test certificate was successful even though the OCSP responder was not yet started.\n"
           "This may happen if the unit test was run twice within one minute.");
    } else {
      CHECK(res_without_responder == CRYPT_E_REVOCATION_OFFLINE);
    }

    // success case: cert_ocsp is fine
    auto ocsp_responder = start_ocsp_responder();
    // 'running()' does not mean that it is responding yet, but that we did start the process correctly
    CHECK(ocsp_responder.running());
    // It takes a few seconds for the OCSP responder to become available.
    // Therefore, we try to verify the certificate in a loop, but for at most 30 seconds.
    auto res_with_responder = CRYPT_E_REVOCATION_OFFLINE;
    const auto start = std::chrono::system_clock::now();
    while (res_with_responder == CRYPT_E_REVOCATION_OFFLINE) {
      res_with_responder = ctx_certs.verify_certificate(cert_ocsp.get(), "", true);
      if (std::chrono::system_clock::now() - start > std::chrono::seconds(30)) {
        WARN("OCSP responder did not provide a response within 30 seconds.");
        break;
      }
    }
    CHECK(res_with_responder == ERROR_SUCCESS);

    // fail case: cert_ocsp_revoked is revoked
    const auto cert_ocsp_revoked = load_chain_file(TEST_CERTIFICATES_PATH "leaf_ocsp_revoked_chain.pem");
    CHECK(ctx_certs.verify_certificate(cert_ocsp_revoked.get(), "", true) == CRYPT_E_REVOKED);
  }
}
