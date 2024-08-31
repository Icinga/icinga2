//
// Copyright (c) 2021 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_CONTEXT_CERTIFICATES_HPP
#define BOOST_WINTLS_DETAIL_CONTEXT_CERTIFICATES_HPP

#include <boost/wintls/detail/config.hpp>

#include <boost/wintls/certificate.hpp>
#include <boost/wintls/error.hpp>

#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>

namespace boost {
namespace wintls {
namespace detail {

struct cert_store_deleter {
  void operator()(HCERTSTORE store) {
    CertCloseStore(store, 0);
  }
};

using cert_store_ptr = std::unique_ptr<std::remove_pointer_t<HCERTSTORE>, cert_store_deleter>;

class context_certificates {
public:
  void add_certificate_authority(const CERT_CONTEXT* cert) {
    init_cert_store();
    if(!CertAddCertificateContextToStore(cert_store_.get(),
                                         cert,
                                         CERT_STORE_ADD_ALWAYS,
                                         nullptr)) {
      throw_last_error("CertAddCertificateContextToStore");
    }
  }

  void add_crl(const CRL_CONTEXT* crl_ctx) {
    init_cert_store();
    if (!CertAddCRLContextToStore(cert_store_.get(),
                                  crl_ctx,
                                  CERT_STORE_ADD_ALWAYS,
                                  nullptr)) {
      throw_last_error("CertAddCRLContextToStore");
    }
  }

  HRESULT verify_certificate(const CERT_CONTEXT* cert, const std::string& server_hostname, bool check_revocation) {
    HRESULT status = CERT_E_UNTRUSTEDROOT;

    if (cert_store_) {
      CERT_CHAIN_ENGINE_CONFIG chain_engine_config{};
      chain_engine_config.cbSize = sizeof(chain_engine_config);
      chain_engine_config.hExclusiveRoot = cert_store_.get();

      struct cert_chain_engine {
        ~cert_chain_engine() {
          CertFreeCertificateChainEngine(ptr);
        }
        HCERTCHAINENGINE ptr = nullptr;
      } chain_engine;

      if (!CertCreateCertificateChainEngine(&chain_engine_config, &chain_engine.ptr)) {
        return static_cast<HRESULT>(GetLastError());
      }

      status = static_cast<HRESULT>(verify_certificate_chain(cert, chain_engine.ptr, server_hostname, check_revocation));
    }

    if (status != ERROR_SUCCESS && use_default_cert_store) {
      // Calling CertGetCertificateChain with a NULL pointer engine uses
      // the default system certificate store
      status = static_cast<HRESULT>(verify_certificate_chain(cert, nullptr, server_hostname, check_revocation));
    }

    return status;
  }

  void use_certificate(const CERT_CONTEXT* cert) {
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE unused_0;
    DWORD unused_1;
    BOOL unused_2;
    if (!CryptAcquireCertificatePrivateKey(cert,
                                           CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
                                           nullptr,
                                           &unused_0,
                                           &unused_1,
                                           &unused_2)) {
      detail::throw_last_error("CryptAcquireCertificatePrivateKey");
    }
    server_cert_ = cert_context_ptr{CertDuplicateCertificateContext(cert)};
  }

  const CERT_CONTEXT* server_cert() const {
    return server_cert_.get();
  }

  bool use_default_cert_store = false;

private:
  void init_cert_store() {
    if (!cert_store_) {
      cert_store_ = cert_store_ptr{CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, 0, nullptr)};
      if (!cert_store_) {
        throw_last_error("CertOpenStore");
      }
    }
  }

  DWORD verify_certificate_chain(const CERT_CONTEXT* cert,
                                 HCERTCHAINENGINE engine,
                                 const std::string& server_hostname,
                                 bool check_revocation) {
    CERT_CHAIN_PARA chain_parameters{};
    chain_parameters.cbSize = sizeof(chain_parameters);

    const CERT_CHAIN_CONTEXT* chain_ctx_ptr;
    DWORD chain_flags = check_revocation ? CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT
                                         : 0;
    if(!CertGetCertificateChain(engine,
                                cert,
                                nullptr,
                                cert->hCertStore,
                                &chain_parameters,
                                chain_flags,
                                nullptr,
                                &chain_ctx_ptr)) {
      return GetLastError();
    }

    std::unique_ptr<const CERT_CHAIN_CONTEXT, decltype(&CertFreeCertificateChain)>
      scoped_chain_ctx{chain_ctx_ptr, &CertFreeCertificateChain};
    // We could return here if (scoped_chain_ctx->TrustStatus.dwErrorStatus != CERT_TRUST_NO_ERROR),
    // but we would have to map the error code to a system error ourselves,.
    // Instead we rely on CertVerifyCertificateChainPolicy to obtain the appropriate system error.

    HTTPSPolicyCallbackData https_policy{};
    https_policy.cbStruct = sizeof(https_policy);
    https_policy.dwAuthType = AUTHTYPE_SERVER;
    // add parameter to verify the host name if it was actually set
    std::wstring whostname;
    if (!server_hostname.empty()) {
      whostname.resize(server_hostname.size(), L' ');
      whostname.resize(std::mbstowcs(&whostname[0], server_hostname.data(), server_hostname.size()));
      https_policy.pwszServerName = &whostname[0];
    }

    CERT_CHAIN_POLICY_PARA policy_params{};
    policy_params.cbSize = sizeof(policy_params);
    policy_params.pvExtraPolicyPara = &https_policy;

    CERT_CHAIN_POLICY_STATUS policy_status{};
    policy_status.cbSize = sizeof(policy_status);

    if(!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
                                         scoped_chain_ctx.get(),
                                         &policy_params,
                                         &policy_status)) {
      return GetLastError();
    }

    return policy_status.dwError;
  }

  cert_store_ptr cert_store_{};
  cert_context_ptr server_cert_{};
};

} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_CONTEXT_CERTIFICATES_HPP
