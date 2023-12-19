/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TLSUTILITY_H
#define TLSUTILITY_H

#include "base/i2-base.hpp"
#include "base/debuginfo.hpp"
#include "base/object.hpp"
#include "base/shared.hpp"
#include "base/array.hpp"
#include "base/string.hpp"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/asio/ssl/context.hpp>
#include <boost/exception/info.hpp>

namespace icinga
{

// Source: https://ssl-config.mozilla.org, i.e.
// ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-CHACHA20-POLY1305
// Modified so that AES256 is preferred over AES128.
const char * const DEFAULT_TLS_CIPHERS = "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256";

const char * const DEFAULT_TLS_PROTOCOLMIN = "TLSv1.2";
const unsigned int DEFAULT_CONNECT_TIMEOUT = 15;

const auto ROOT_VALID_FOR  = 60 * 60 * 24 * 365 * 15;
const auto LEAF_VALID_FOR  = 60 * 60 * 24 * 397;
const auto RENEW_THRESHOLD = 60 * 60 * 24 * 30;
const auto RENEW_INTERVAL  = 60 * 60 * 24;

void InitializeOpenSSL();

String GetOpenSSLVersion();

Shared<boost::asio::ssl::context>::Ptr MakeAsioSslContext(const String& pubkey = String(), const String& privkey = String(), const String& cakey = String());
void AddCRLToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& crlPath);
void AddCRLToSSLContext(X509_STORE *x509_store, const String& crlPath);
void SetCipherListToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& cipherList);
void SetTlsProtocolminToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& tlsProtocolmin);
int ResolveTlsProtocolVersion(const std::string& version);

Shared<boost::asio::ssl::context>::Ptr SetupSslContext(String certPath, String keyPath,
	String caPath, String crlPath, String cipherList, String protocolmin, DebugInfo di);

String GetCertificateCN(const std::shared_ptr<X509>& certificate);
std::shared_ptr<X509> GetX509Certificate(const String& pemfile);
int MakeX509CSR(const String& cn, const String& keyfile, const String& csrfile = String(), const String& certfile = String(), bool ca = false);
std::shared_ptr<X509> CreateCert(EVP_PKEY *pubkey, X509_NAME *subject, X509_NAME *issuer, EVP_PKEY *cakey, bool ca);

String GetIcingaCADir();
String CertificateToString(X509* cert);

inline String CertificateToString(const std::shared_ptr<X509>& cert)
{
	return CertificateToString(cert.get());
}

std::shared_ptr<X509> StringToCertificate(const String& cert);
std::shared_ptr<X509> CreateCertIcingaCA(EVP_PKEY *pubkey, X509_NAME *subject, bool ca = false);
std::shared_ptr<X509> CreateCertIcingaCA(const std::shared_ptr<X509>& cert);
bool IsCertUptodate(const std::shared_ptr<X509>& cert);
bool IsCaUptodate(X509* cert);

String PBKDF2_SHA1(const String& password, const String& salt, int iterations);
String PBKDF2_SHA256(const String& password, const String& salt, int iterations);
String SHA1(const String& s, bool binary = false);
String SHA256(const String& s);
String RandomString(int length);
String BinaryToHex(const unsigned char* data, size_t length);

bool VerifyCertificate(const std::shared_ptr<X509>& caCertificate, const std::shared_ptr<X509>& certificate, const String& crlFile);
bool IsCa(const std::shared_ptr<X509>& cacert);
int GetCertificateVersion(const std::shared_ptr<X509>& cert);
String GetSignatureAlgorithm(const std::shared_ptr<X509>& cert);
Array::Ptr GetSubjectAltNames(const std::shared_ptr<X509>& cert);

class openssl_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_openssl_error_;
typedef boost::error_info<struct errinfo_openssl_error_, unsigned long> errinfo_openssl_error;

}

#endif /* TLSUTILITY_H */
