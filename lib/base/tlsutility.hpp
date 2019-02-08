/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TLSUTILITY_H
#define TLSUTILITY_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/string.hpp"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/comp.h>
#include <openssl/sha.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/asio/ssl/context.hpp>
#include <boost/exception/info.hpp>

namespace icinga
{

void InitializeOpenSSL();
std::shared_ptr<SSL_CTX> MakeSSLContext(const String& pubkey = String(), const String& privkey = String(), const String& cakey = String());
std::shared_ptr<boost::asio::ssl::context> MakeAsioSslContext(const String& pubkey = String(), const String& privkey = String(), const String& cakey = String());
void AddCRLToSSLContext(const std::shared_ptr<boost::asio::ssl::context>& context, const String& crlPath);
void SetCipherListToSSLContext(const std::shared_ptr<boost::asio::ssl::context>& context, const String& cipherList);
void SetTlsProtocolminToSSLContext(const std::shared_ptr<boost::asio::ssl::context>& context, const String& tlsProtocolmin);
String GetCertificateCN(const std::shared_ptr<X509>& certificate);
std::shared_ptr<X509> GetX509Certificate(const String& pemfile);
int MakeX509CSR(const String& cn, const String& keyfile, const String& csrfile = String(), const String& certfile = String(), bool ca = false);
std::shared_ptr<X509> CreateCert(EVP_PKEY *pubkey, X509_NAME *subject, X509_NAME *issuer, EVP_PKEY *cakey, bool ca);
String GetIcingaCADir();
String CertificateToString(const std::shared_ptr<X509>& cert);
std::shared_ptr<X509> StringToCertificate(const String& cert);
std::shared_ptr<X509> CreateCertIcingaCA(EVP_PKEY *pubkey, X509_NAME *subject);
std::shared_ptr<X509> CreateCertIcingaCA(const std::shared_ptr<X509>& cert);
String PBKDF2_SHA1(const String& password, const String& salt, int iterations);
String PBKDF2_SHA256(const String& password, const String& salt, int iterations);
String SHA1(const String& s, bool binary = false);
String SHA256(const String& s);
String RandomString(int length);
bool VerifyCertificate(const std::shared_ptr<X509>& caCertificate, const std::shared_ptr<X509>& certificate);

class openssl_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_openssl_error_;
typedef boost::error_info<struct errinfo_openssl_error_, unsigned long> errinfo_openssl_error;

std::string to_string(const errinfo_openssl_error& e);

}

#endif /* TLSUTILITY_H */
