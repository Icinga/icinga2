/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/tlsutility.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include <boost/asio/ssl/context.hpp>
#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <fstream>

namespace icinga
{

static bool l_SSLInitialized = false;
static std::mutex *l_Mutexes;
static std::mutex l_RandomMutex;

String GetOpenSSLVersion()
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	return OpenSSL_version(OPENSSL_VERSION);
#else /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
	return SSLeay_version(SSLEAY_VERSION);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
}

#ifdef CRYPTO_LOCK
static void OpenSSLLockingCallback(int mode, int type, const char *, int)
{
	if (mode & CRYPTO_LOCK)
		l_Mutexes[type].lock();
	else
		l_Mutexes[type].unlock();
}

static unsigned long OpenSSLIDCallback()
{
#ifdef _WIN32
	return (unsigned long)GetCurrentThreadId();
#else /* _WIN32 */
	return (unsigned long)pthread_self();
#endif /* _WIN32 */
}
#endif /* CRYPTO_LOCK */

/**
 * Initializes the OpenSSL library.
 */
void InitializeOpenSSL()
{
	if (l_SSLInitialized)
		return;

	SSL_library_init();
	SSL_load_error_strings();

	SSL_COMP_get_compression_methods();

#ifdef CRYPTO_LOCK
	l_Mutexes = new std::mutex[CRYPTO_num_locks()];
	CRYPTO_set_locking_callback(&OpenSSLLockingCallback);
	CRYPTO_set_id_callback(&OpenSSLIDCallback);
#endif /* CRYPTO_LOCK */

	l_SSLInitialized = true;
}

static void SetupSslContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& pubkey, const String& privkey, const String& cakey)
{
	char errbuf[256];

	// Enforce TLS v1.2 as minimum
	context->set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_compression |
		boost::asio::ssl::context::no_sslv2 |
		boost::asio::ssl::context::no_sslv3 |
		boost::asio::ssl::context::no_tlsv1 |
		boost::asio::ssl::context::no_tlsv1_1
	);

	// Custom TLS flags
	SSL_CTX *sslContext = context->native_handle();

	long flags = SSL_CTX_get_options(sslContext);

	flags |= SSL_OP_CIPHER_SERVER_PREFERENCE;

	SSL_CTX_set_options(sslContext, flags);

	SSL_CTX_set_mode(sslContext, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	SSL_CTX_set_session_id_context(sslContext, (const unsigned char *)"Icinga 2", 8);

	// Explicitly load ECC ciphers, required on el7 - https://github.com/Icinga/icinga2/issues/7247
	// SSL_CTX_set_ecdh_auto is deprecated and removed in OpenSSL 1.1.x - https://github.com/openssl/openssl/issues/1437
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#	ifdef SSL_CTX_set_ecdh_auto
	SSL_CTX_set_ecdh_auto(sslContext, 1);
#	endif /* SSL_CTX_set_ecdh_auto */
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

	if (!pubkey.IsEmpty()) {
		if (!SSL_CTX_use_certificate_chain_file(sslContext, pubkey.CStr())) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error with public key file '" << pubkey << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("SSL_CTX_use_certificate_chain_file")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(pubkey));
		}
	}

	if (!privkey.IsEmpty()) {
		if (!SSL_CTX_use_PrivateKey_file(sslContext, privkey.CStr(), SSL_FILETYPE_PEM)) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error with private key file '" << privkey << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("SSL_CTX_use_PrivateKey_file")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(privkey));
		}

		if (!SSL_CTX_check_private_key(sslContext)) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error checking private key '" << privkey << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("SSL_CTX_check_private_key")
				<< errinfo_openssl_error(ERR_peek_error()));
		}
	}

	if (!cakey.IsEmpty()) {
		if (!SSL_CTX_load_verify_locations(sslContext, cakey.CStr(), nullptr)) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error loading and verifying locations in ca key file '" << cakey << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("SSL_CTX_load_verify_locations")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(cakey));
		}

		STACK_OF(X509_NAME) *cert_names;

		cert_names = SSL_load_client_CA_file(cakey.CStr());
		if (!cert_names) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error loading client ca key file '" << cakey << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("SSL_load_client_CA_file")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(cakey));
		}

		SSL_CTX_set_client_CA_list(sslContext, cert_names);
	}
}

/**
 * Initializes an SSL context using the specified certificates.
 *
 * @param pubkey The public key.
 * @param privkey The matching private key.
 * @param cakey CA certificate chain file.
 * @returns An SSL context.
 */
Shared<boost::asio::ssl::context>::Ptr MakeAsioSslContext(const String& pubkey, const String& privkey, const String& cakey)
{
	namespace ssl = boost::asio::ssl;

	InitializeOpenSSL();

	auto context (Shared<ssl::context>::Make(ssl::context::tlsv12));

	SetupSslContext(context, pubkey, privkey, cakey);

	return context;
}

/**
 * Set the cipher list to the specified SSL context.
 * @param context The ssl context.
 * @param cipherList The ciper list.
 **/
void SetCipherListToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& cipherList)
{
	char errbuf[256];

	if (SSL_CTX_set_cipher_list(context->native_handle(), cipherList.CStr()) == 0) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Cipher list '"
			<< cipherList
			<< "' does not specify any usable ciphers: "
			<< ERR_peek_error() << ", \""
			<< errbuf << "\"";

		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SSL_CTX_set_cipher_list")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	//With OpenSSL 1.1.0, there might not be any returned 0.
	STACK_OF(SSL_CIPHER) *ciphers;
	Array::Ptr cipherNames = new Array();

	ciphers = SSL_CTX_get_ciphers(context->native_handle());
	for (int i = 0; i < sk_SSL_CIPHER_num(ciphers); i++) {
		const SSL_CIPHER *cipher = sk_SSL_CIPHER_value(ciphers, i);
		String cipher_name = SSL_CIPHER_get_name(cipher);

		cipherNames->Add(cipher_name);
	}

	Log(LogNotice, "TlsUtility")
		<< "Available TLS cipher list: " << cipherNames->Join(" ");
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
}

/**
 * Set the minimum TLS protocol version to the specified SSL context.
 *
 * @param context The ssl context.
 * @param tlsProtocolmin The minimum TLS protocol version.
 */
void SetTlsProtocolminToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& tlsProtocolmin)
{
	// tlsProtocolmin has no effect since we enforce TLS 1.2 since 2.11.
	/*
	std::shared_ptr<SSL_CTX> sslContext = std::shared_ptr<SSL_CTX>(context->native_handle());

	long flags = SSL_CTX_get_options(sslContext.get());

	flags |= ...;

	SSL_CTX_set_options(sslContext.get(), flags);
	*/
}

/**
 * Loads a CRL and appends its certificates to the specified Boost SSL context.
 *
 * @param context The SSL context.
 * @param crlPath The path to the CRL file.
 */
void AddCRLToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& crlPath)
{
	X509_STORE *x509_store = SSL_CTX_get_cert_store(context->native_handle());
	AddCRLToSSLContext(x509_store, crlPath);
}

/**
 * Loads a CRL and appends its certificates to the specified OpenSSL X509 store.
 *
 * @param context The SSL context.
 * @param crlPath The path to the CRL file.
 */
void AddCRLToSSLContext(X509_STORE *x509_store, const String& crlPath)
{
	char errbuf[256];

	X509_LOOKUP *lookup;
	lookup = X509_STORE_add_lookup(x509_store, X509_LOOKUP_file());

	if (!lookup) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error adding X509 store lookup: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("X509_STORE_add_lookup")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (X509_LOOKUP_load_file(lookup, crlPath.CStr(), X509_FILETYPE_PEM) != 1) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error loading crl file '" << crlPath << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("X509_LOOKUP_load_file")
			<< errinfo_openssl_error(ERR_peek_error())
			<< boost::errinfo_file_name(crlPath));
	}

	X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
	X509_STORE_set1_param(x509_store, param);
	X509_VERIFY_PARAM_free(param);
}

static String GetX509NameCN(X509_NAME *name)
{
	char errbuf[256];
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(name, NID_commonName, buffer, sizeof(buffer));

	if (rc == -1) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error with x509 NAME getting text by NID: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("X509_NAME_get_text_by_NID")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	return buffer;
}

/**
 * Retrieves the common name for an X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
String GetCertificateCN(const std::shared_ptr<X509>& certificate)
{
	return GetX509NameCN(X509_get_subject_name(certificate.get()));
}

/**
 * Retrieves an X509 certificate from the specified file.
 *
 * @param pemfile The filename.
 * @returns An X509 certificate.
 */
std::shared_ptr<X509> GetX509Certificate(const String& pemfile)
{
	char errbuf[256];
	X509 *cert;
	BIO *fpcert = BIO_new(BIO_s_file());

	if (!fpcert) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error creating new BIO: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("BIO_new")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (BIO_read_filename(fpcert, pemfile.CStr()) < 0) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error reading pem file '" << pemfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("BIO_read_filename")
			<< errinfo_openssl_error(ERR_peek_error())
			<< boost::errinfo_file_name(pemfile));
	}

	cert = PEM_read_bio_X509_AUX(fpcert, nullptr, nullptr, nullptr);
	if (!cert) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on bio X509 AUX reading pem file '" << pemfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("PEM_read_bio_X509_AUX")
			<< errinfo_openssl_error(ERR_peek_error())
			<< boost::errinfo_file_name(pemfile));
	}

	BIO_free(fpcert);

	return std::shared_ptr<X509>(cert, X509_free);
}

int MakeX509CSR(const String& cn, const String& keyfile, const String& csrfile, const String& certfile, bool ca)
{
	char errbuf[256];

	InitializeOpenSSL();

	RSA *rsa = RSA_new();
	BIGNUM *e = BN_new();

	if (!rsa || !e) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error while creating RSA key: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("RSA_generate_key")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	BN_set_word(e, RSA_F4);

	if (!RSA_generate_key_ex(rsa, 4096, e, nullptr)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error while creating RSA key: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("RSA_generate_key")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	BN_free(e);

	Log(LogInformation, "base")
		<< "Writing private key to '" << keyfile << "'.";

	BIO *bio = BIO_new_file(const_cast<char *>(keyfile.CStr()), "w");

	if (!bio) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error while opening private RSA key file '" << keyfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("BIO_new_file")
			<< errinfo_openssl_error(ERR_peek_error())
			<< boost::errinfo_file_name(keyfile));
	}

	if (!PEM_write_bio_RSAPrivateKey(bio, rsa, nullptr, nullptr, 0, nullptr, nullptr)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error while writing private RSA key to file '" << keyfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("PEM_write_bio_RSAPrivateKey")
			<< errinfo_openssl_error(ERR_peek_error())
			<< boost::errinfo_file_name(keyfile));
	}

	BIO_free(bio);

#ifndef _WIN32
	chmod(keyfile.CStr(), 0600);
#endif /* _WIN32 */

	EVP_PKEY *key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(key, rsa);

	if (!certfile.IsEmpty()) {
		X509_NAME *subject = X509_NAME_new();
		X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC, (unsigned char *)cn.CStr(), -1, -1, 0);

		std::shared_ptr<X509> cert = CreateCert(key, subject, subject, key, ca);

		X509_NAME_free(subject);

		Log(LogInformation, "base")
			<< "Writing X509 certificate to '" << certfile << "'.";

		bio = BIO_new_file(const_cast<char *>(certfile.CStr()), "w");

		if (!bio) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error while opening certificate file '" << certfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("BIO_new_file")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(certfile));
		}

		if (!PEM_write_bio_X509(bio, cert.get())) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error while writing certificate to file '" << certfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("PEM_write_bio_X509")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(certfile));
		}

		BIO_free(bio);
	}

	if (!csrfile.IsEmpty()) {
		X509_REQ *req = X509_REQ_new();

		if (!req)
			return 0;

		X509_REQ_set_version(req, 0);
		X509_REQ_set_pubkey(req, key);

		X509_NAME *name = X509_REQ_get_subject_name(req);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cn.CStr(), -1, -1, 0);

		if (!ca) {
			String san = "DNS:" + cn;
			X509_EXTENSION *subjectAltNameExt = X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name, const_cast<char *>(san.CStr()));
			if (subjectAltNameExt) {
				/* OpenSSL 0.9.8 requires STACK_OF(X509_EXTENSION), otherwise we would just use stack_st_X509_EXTENSION. */
				STACK_OF(X509_EXTENSION) *exts = sk_X509_EXTENSION_new_null();
				sk_X509_EXTENSION_push(exts, subjectAltNameExt);
				X509_REQ_add_extensions(req, exts);
				sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
			}
		}

		X509_REQ_sign(req, key, EVP_sha256());

		Log(LogInformation, "base")
			<< "Writing certificate signing request to '" << csrfile << "'.";

		bio = BIO_new_file(const_cast<char *>(csrfile.CStr()), "w");

		if (!bio) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error while opening CSR file '" << csrfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("BIO_new_file")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(csrfile));
		}

		if (!PEM_write_bio_X509_REQ(bio, req)) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error while writing CSR to file '" << csrfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("PEM_write_bio_X509")
				<< errinfo_openssl_error(ERR_peek_error())
				<< boost::errinfo_file_name(csrfile));
		}

		BIO_free(bio);

		X509_REQ_free(req);
	}

	EVP_PKEY_free(key);

	return 1;
}

std::shared_ptr<X509> CreateCert(EVP_PKEY *pubkey, X509_NAME *subject, X509_NAME *issuer, EVP_PKEY *cakey, bool ca)
{
	X509 *cert = X509_new();
	X509_set_version(cert, 2);
	X509_gmtime_adj(X509_get_notBefore(cert), 0);
	X509_gmtime_adj(X509_get_notAfter(cert), 365 * 24 * 60 * 60 * 15);
	X509_set_pubkey(cert, pubkey);

	X509_set_subject_name(cert, subject);
	X509_set_issuer_name(cert, issuer);

	String id = Utility::NewUniqueID();

	char errbuf[256];
	SHA_CTX context;
	unsigned char digest[SHA_DIGEST_LENGTH];

	if (!SHA1_Init(&context)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA1 Init: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA1_Init")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA1_Update(&context, (unsigned char*)id.CStr(), id.GetLength())) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA1 Update: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA1_Update")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA1_Final(digest, &context)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA1 Final: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA1_Final")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	BIGNUM *bn = BN_new();
	BN_bin2bn(digest, sizeof(digest), bn);
	BN_to_ASN1_INTEGER(bn, X509_get_serialNumber(cert));
	BN_free(bn);

	X509V3_CTX ctx;
	X509V3_set_ctx_nodb(&ctx);
	X509V3_set_ctx(&ctx, cert, cert, nullptr, nullptr, 0);

	const char *attr;

	if (ca)
		attr = "critical,CA:TRUE";
	else
		attr = "critical,CA:FALSE";

	X509_EXTENSION *basicConstraintsExt = X509V3_EXT_conf_nid(nullptr, &ctx, NID_basic_constraints, const_cast<char *>(attr));

	if (basicConstraintsExt) {
		X509_add_ext(cert, basicConstraintsExt, -1);
		X509_EXTENSION_free(basicConstraintsExt);
	}

	String cn = GetX509NameCN(subject);

	if (!ca) {
		String san = "DNS:" + cn;
		X509_EXTENSION *subjectAltNameExt = X509V3_EXT_conf_nid(nullptr, &ctx, NID_subject_alt_name, const_cast<char *>(san.CStr()));
		if (subjectAltNameExt) {
			X509_add_ext(cert, subjectAltNameExt, -1);
			X509_EXTENSION_free(subjectAltNameExt);
		}
	}

	X509_sign(cert, cakey, EVP_sha256());

	return std::shared_ptr<X509>(cert, X509_free);
}

String GetIcingaCADir()
{
	return Configuration::DataDir + "/ca";
}

std::shared_ptr<X509> CreateCertIcingaCA(EVP_PKEY *pubkey, X509_NAME *subject)
{
	char errbuf[256];

	String cadir = GetIcingaCADir();

	String cakeyfile = cadir + "/ca.key";

	RSA *rsa;

	BIO *cakeybio = BIO_new_file(const_cast<char *>(cakeyfile.CStr()), "r");

	if (!cakeybio) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Could not open CA key file '" << cakeyfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		return std::shared_ptr<X509>();
	}

	rsa = PEM_read_bio_RSAPrivateKey(cakeybio, nullptr, nullptr, nullptr);

	if (!rsa) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Could not read RSA key from CA key file '" << cakeyfile << "': " << ERR_peek_error() << ", \"" << errbuf << "\"";
		return std::shared_ptr<X509>();
	}

	BIO_free(cakeybio);

	String cacertfile = cadir + "/ca.crt";

	std::shared_ptr<X509> cacert = GetX509Certificate(cacertfile);

	EVP_PKEY *privkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(privkey, rsa);

	return CreateCert(pubkey, subject, X509_get_subject_name(cacert.get()), privkey, false);
}

std::shared_ptr<X509> CreateCertIcingaCA(const std::shared_ptr<X509>& cert)
{
	std::shared_ptr<EVP_PKEY> pkey = std::shared_ptr<EVP_PKEY>(X509_get_pubkey(cert.get()), EVP_PKEY_free);
	return CreateCertIcingaCA(pkey.get(), X509_get_subject_name(cert.get()));
}

String CertificateToString(const std::shared_ptr<X509>& cert)
{
	BIO *mem = BIO_new(BIO_s_mem());
	PEM_write_bio_X509(mem, cert.get());

	char *data;
	long len = BIO_get_mem_data(mem, &data);

	String result = String(data, data + len);

	BIO_free(mem);

	return result;
}

std::shared_ptr<X509> StringToCertificate(const String& cert)
{
	BIO *bio = BIO_new(BIO_s_mem());
	BIO_write(bio, (const void *)cert.CStr(), cert.GetLength());

	X509 *rawCert = PEM_read_bio_X509_AUX(bio, nullptr, nullptr, nullptr);

	BIO_free(bio);

	if (!rawCert)
		BOOST_THROW_EXCEPTION(std::invalid_argument("The specified X509 certificate is invalid."));

	return std::shared_ptr<X509>(rawCert, X509_free);
}

String PBKDF2_SHA1(const String& password, const String& salt, int iterations)
{
	unsigned char digest[SHA_DIGEST_LENGTH];
	PKCS5_PBKDF2_HMAC_SHA1(password.CStr(), password.GetLength(), reinterpret_cast<const unsigned char *>(salt.CStr()), salt.GetLength(),
		iterations, sizeof(digest), digest);

	char output[SHA_DIGEST_LENGTH*2+1];
	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
		sprintf(output + 2 * i, "%02x", digest[i]);

	return output;
}

String PBKDF2_SHA256(const String& password, const String& salt, int iterations)
{
	unsigned char digest[SHA256_DIGEST_LENGTH];
	PKCS5_PBKDF2_HMAC(password.CStr(), password.GetLength(), reinterpret_cast<const unsigned char *>(salt.CStr()),
		salt.GetLength(), iterations, EVP_sha256(), SHA256_DIGEST_LENGTH, digest);

	char output[SHA256_DIGEST_LENGTH*2+1];
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		sprintf(output + 2 * i, "%02x", digest[i]);

	return output;
}

String SHA1(const String& s, bool binary)
{
	char errbuf[256];
	SHA_CTX context;
	unsigned char digest[SHA_DIGEST_LENGTH];

	if (!SHA1_Init(&context)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA Init: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA1_Init")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA1_Update(&context, (unsigned char*)s.CStr(), s.GetLength())) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA Update: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA1_Update")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA1_Final(digest, &context)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA Final: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA1_Final")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (binary)
		return String(reinterpret_cast<const char*>(digest), reinterpret_cast<const char *>(digest + SHA_DIGEST_LENGTH));

	char output[SHA_DIGEST_LENGTH*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", digest[i]);

	return output;
}

String SHA256(const String& s)
{
	char errbuf[256];
	SHA256_CTX context;
	unsigned char digest[SHA256_DIGEST_LENGTH];

	if (!SHA256_Init(&context)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA256 Init: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Init")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA256_Update(&context, (unsigned char*)s.CStr(), s.GetLength())) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA256 Update: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Update")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA256_Final(digest, &context)) {
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error on SHA256 Final: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Final")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	char output[SHA256_DIGEST_LENGTH*2+1];
	for (int i = 0; i < 32; i++)
		sprintf(output + 2 * i, "%02x", digest[i]);

	return output;
}

String RandomString(int length)
{
	auto *bytes = new unsigned char[length];

	/* Ensure that password generation is atomic. RAND_bytes is not thread-safe
	 * in OpenSSL < 1.1.0.
	 */
	std::unique_lock<std::mutex> lock(l_RandomMutex);

	if (!RAND_bytes(bytes, length)) {
		delete [] bytes;

		char errbuf[256];
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);

		Log(LogCritical, "SSL")
			<< "Error for RAND_bytes: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("RAND_bytes")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	lock.unlock();

	auto *output = new char[length * 2 + 1];
	for (int i = 0; i < length; i++)
		sprintf(output + 2 * i, "%02x", bytes[i]);

	String result = output;
	delete [] bytes;
	delete [] output;

	return result;
}

bool VerifyCertificate(const std::shared_ptr<X509> &caCertificate, const std::shared_ptr<X509> &certificate, const String& crlFile)
{
	X509_STORE *store = X509_STORE_new();

	if (!store)
		return false;

	X509_STORE_add_cert(store, caCertificate.get());

	if (!crlFile.IsEmpty()) {
		AddCRLToSSLContext(store, crlFile);
	}

	X509_STORE_CTX *csc = X509_STORE_CTX_new();
	X509_STORE_CTX_init(csc, store, certificate.get(), nullptr);

	int rc = X509_verify_cert(csc);

	X509_STORE_CTX_free(csc);
	X509_STORE_free(store);

	if (rc == 0) {
		int err = X509_STORE_CTX_get_error(csc);

		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("X509_verify_cert")
			<< errinfo_openssl_error(err));
	}

	return rc == 1;
}

bool IsCa(const std::shared_ptr<X509>& cacert)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	/* OpenSSL 1.1.x provides https://www.openssl.org/docs/man1.1.0/man3/X509_check_ca.html
	 *
	 * 0 if it is not CA certificate,
	 * 1 if it is proper X509v3 CA certificate with basicConstraints extension CA:TRUE,
	 * 3 if it is self-signed X509 v1 certificate
	 * 4 if it is certificate with keyUsage extension with bit keyCertSign set, but without basicConstraints,
	 * 5 if it has outdated Netscape Certificate Type extension telling that it is CA certificate.
	 */
	return (X509_check_ca(cacert.get()) == 1);
#else /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
	BOOST_THROW_EXCEPTION(std::invalid_argument("Not supported on this platform, OpenSSL version too old."));
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
}

int GetCertificateVersion(const std::shared_ptr<X509>& cert)
{
	return X509_get_version(cert.get()) + 1;
}

String GetSignatureAlgorithm(const std::shared_ptr<X509>& cert)
{
	int alg;
	int sign_alg;
	X509_PUBKEY *key;
	X509_ALGOR *algor;

	key = X509_get_X509_PUBKEY(cert.get());

	X509_PUBKEY_get0_param(nullptr, nullptr, 0, &algor, key); //TODO: Error handling

	alg = OBJ_obj2nid (algor->algorithm);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	sign_alg = OBJ_obj2nid((cert.get())->sig_alg->algorithm);
#else /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	sign_alg = X509_get_signature_nid(cert.get());
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

	return Convert::ToString((sign_alg == NID_undef) ? "Unknown" : OBJ_nid2ln(sign_alg));
}

Array::Ptr GetSubjectAltNames(const std::shared_ptr<X509>& cert)
{
	GENERAL_NAMES* subjectAltNames = (GENERAL_NAMES*)X509_get_ext_d2i(cert.get(), NID_subject_alt_name, nullptr, nullptr);

	Array::Ptr sans = new Array();

	for (int i = 0; i < sk_GENERAL_NAME_num(subjectAltNames); i++) {
		GENERAL_NAME* gen = sk_GENERAL_NAME_value(subjectAltNames, i);
		if (gen->type == GEN_URI || gen->type == GEN_DNS || gen->type == GEN_EMAIL) {
			ASN1_IA5STRING *asn1_str = gen->d.uniformResourceIdentifier;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
			String san = Convert::ToString(ASN1_STRING_data(asn1_str));
#else /* OPENSSL_VERSION_NUMBER < 0x10100000L */
			String san = Convert::ToString(ASN1_STRING_get0_data(asn1_str));
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

			sans->Add(san);
		}
	}

	GENERAL_NAMES_free(subjectAltNames);

	return sans;
}

}
