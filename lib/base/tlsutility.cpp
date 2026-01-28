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
#include <openssl/ssl.h>
#include <openssl/ssl3.h>
#include <fstream>

namespace icinga
{

static bool l_SSLInitialized = false;
static std::mutex l_RandomMutex;

String GetOpenSSLVersion()
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	return OpenSSL_version(OPENSSL_VERSION);
#else /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
	return SSLeay_version(SSLEAY_VERSION);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static std::mutex *l_Mutexes;

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
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

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

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	l_Mutexes = new std::mutex[CRYPTO_num_locks()];
	CRYPTO_set_locking_callback(&OpenSSLLockingCallback);
	CRYPTO_set_id_callback(&OpenSSLIDCallback);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

	l_SSLInitialized = true;
}

static void InitSslContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& pubkey, const String& privkey, const String& cakey)
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

#ifdef LIBRESSL_VERSION_NUMBER
	flags |= SSL_OP_NO_CLIENT_RENEGOTIATION;
#elif OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_CTX_set_info_callback(sslContext, [](const SSL* ssl, int where, int) {
		if (where & SSL_CB_HANDSHAKE_DONE) {
			ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
		}
	});
#else /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	flags |= SSL_OP_NO_RENEGOTIATION;
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

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

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	// The built-in DH parameters have to be enabled explicitly to allow the use of ciphers that use a DHE key exchange.
	// SSL_CTX_set_dh_auto is only documented in OpenSSL starting from version 3.0.0 but was already added in 1.1.0.
	// https://github.com/openssl/openssl/commit/09599b52d4e295c380512ba39958a11994d63401
	// https://github.com/openssl/openssl/commit/0437309fdf544492e272943e892523653df2f189
	SSL_CTX_set_dh_auto(sslContext, 1);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */

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

	if (cakey.IsEmpty()) {
		if (!SSL_CTX_set_default_verify_paths(sslContext)) {
			ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
			Log(LogCritical, "SSL")
				<< "Error loading system's root CAs: " << ERR_peek_error() << ", \"" << errbuf << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
				<< boost::errinfo_api_function("SSL_CTX_set_default_verify_paths")
				<< errinfo_openssl_error(ERR_peek_error()));
		}
	} else {
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

	auto context (Shared<ssl::context>::Make(ssl::context::tls));

	InitSslContext(context, pubkey, privkey, cakey);

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
 * Resolves a string describing a TLS protocol version to the value of a TLS*_VERSION macro of OpenSSL.
 *
 * Throws an exception if the version is unknown or not supported.
 *
 * @param version String of a TLS version, for example "TLSv1.2".
 * @return The value of the corresponding TLS*_VERSION macro.
 */
int ResolveTlsProtocolVersion(const std::string& version) {
	if (version == "TLSv1.2") {
		return TLS1_2_VERSION;
	} else if (version == "TLSv1.3") {
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
		return TLS1_3_VERSION;
#else /* OPENSSL_VERSION_NUMBER >= 0x10101000L */
		throw std::runtime_error("'" + version + "' is only supported with OpenSSL 1.1.1 or newer");
#endif /* OPENSSL_VERSION_NUMBER >= 0x10101000L */
	} else {
		throw std::runtime_error("Unknown TLS protocol version '" + version + "'");
	}
}

Shared<boost::asio::ssl::context>::Ptr SetupSslContext(String certPath, String keyPath,
	String caPath, String crlPath, String cipherList, String protocolmin, DebugInfo di)
{
	namespace ssl = boost::asio::ssl;

	Shared<ssl::context>::Ptr context;

	try {
		context = MakeAsioSslContext(certPath, keyPath, caPath);
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot make SSL context for cert path: '"
			+ certPath + "' key path: '" + keyPath + "' ca path: '" + caPath + "'.", di));
	}

	if (!crlPath.IsEmpty()) {
		try {
			AddCRLToSSLContext(context, crlPath);
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot add certificate revocation list to SSL context for crl path: '"
				+ crlPath + "'.", di));
		}
	}

	if (!cipherList.IsEmpty()) {
		try {
			SetCipherListToSSLContext(context, cipherList);
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot set cipher list to SSL context for cipher list: '"
				+ cipherList + "'.", di));
		}
	}

	if (!protocolmin.IsEmpty()){
		try {
			SetTlsProtocolminToSSLContext(context, protocolmin);
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot set minimum TLS protocol version to SSL context with tls_protocolmin: '" + protocolmin + "'.", di));
		}
	}

	return context;
}

/**
 * Set the minimum TLS protocol version to the specified SSL context.
 *
 * @param context The ssl context.
 * @param tlsProtocolmin The minimum TLS protocol version.
 */
void SetTlsProtocolminToSSLContext(const Shared<boost::asio::ssl::context>::Ptr& context, const String& tlsProtocolmin)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	int ret = SSL_CTX_set_min_proto_version(context->native_handle(), ResolveTlsProtocolVersion(tlsProtocolmin));

	if (ret != 1) {
		char errbuf[256];

		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL")
			<< "Error setting minimum TLS protocol version: " << ERR_peek_error() << ", \"" << errbuf << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SSL_CTX_set_min_proto_version")
			<< errinfo_openssl_error(ERR_peek_error()));
	}
#else /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
	// This should never happen. On this OpenSSL version, ResolveTlsProtocolVersion() should either return TLS 1.2
	// or throw an exception, as that's the only TLS version supported by both Icinga and ancient OpenSSL.
	VERIFY(ResolveTlsProtocolVersion(tlsProtocolmin) == TLS1_2_VERSION);
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
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

int MakeX509CSR(
	const String& cn,
	const String& keyfile,
	const String& csrfile,
	const String& certfile,
	long validFrom,
	long validFor,
	bool ca
)
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

		std::shared_ptr<X509> cert = CreateCert(key, subject, subject, key, validFrom, validFor, ca);

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

std::shared_ptr<X509> CreateCert(
	EVP_PKEY* pubkey,
	X509_NAME* subject,
	X509_NAME* issuer,
	EVP_PKEY* cakey,
	long validFrom,
	long validFor,
	bool ca
)
{
	X509 *cert = X509_new();
	X509_set_version(cert, 2);
	X509_gmtime_adj(X509_get_notBefore(cert), validFrom);
	X509_gmtime_adj(X509_get_notAfter(cert), validFor);
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

	if (!ca) {
		String san = "DNS:" + GetX509NameCN(subject);
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

std::shared_ptr<X509> CreateCertIcingaCA(EVP_PKEY *pubkey, X509_NAME *subject, long validFrom, long validFor, bool ca)
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

	return CreateCert(pubkey, subject, X509_get_subject_name(cacert.get()), privkey, validFrom, validFor, ca);
}

/**
 * Creates a new X509 certificate signed by the Icinga CA.
 *
 * @param cert The certificate containing the public key and subject name.
 * @param validFor The validity period in seconds. Defaults to LEAF_VALID_FOR.
 * @param validFrom The start time offset in seconds. Defaults to 0 (now).
 * @returns The new X509 certificate or an empty shared_ptr on error.
 */
std::shared_ptr<X509> CreateCertIcingaCA(const std::shared_ptr<X509>& cert, long validFrom, long validFor)
{
	std::shared_ptr<EVP_PKEY> pkey = std::shared_ptr<EVP_PKEY>(X509_get_pubkey(cert.get()), EVP_PKEY_free);
	return CreateCertIcingaCA(pkey.get(), X509_get_subject_name(cert.get()), validFrom, validFor);
}

/**
 * Checks whether the specified certificate expires within the specified number of seconds.
 *
 * @param cert The certificate to its expiration for.
 * @param seconds The number of seconds to check against.
 *
 * @returns True if the certificate expires within the specified number of seconds, false otherwise.
 */
static bool CertExpiresWithin(X509* cert, long seconds)
{
	auto now = time(nullptr);
	std::shared_ptr<ASN1_TIME> renewalStart(X509_time_adj_ex(nullptr, 0, seconds, &now), ASN1_TIME_free);

	return Asn1TimeCompare(X509_get_notAfter(cert), renewalStart.get()) < 0;
}

bool IsCertUptodate(const std::shared_ptr<X509>& cert)
{
	if (CertExpiresWithin(cert.get(), RENEW_THRESHOLD)) {
		return false;
	}

	/* auto-renew all certificates which were created before 2017 to force an update of the CA,
	 * because Icinga versions older than 2.4 sometimes create certificates with an invalid
	 * serial number. */
	time_t forceRenewalEnd = 1483228800; /* January 1st, 2017 */

	return X509_cmp_time(X509_get_notBefore(cert.get()), &forceRenewalEnd) >= 0;
}

bool IsCaUptodate(X509* cert)
{
	return !CertExpiresWithin(cert, LEAF_VALID_FOR);
}

/**
 * Compares two ASN1_TIME values.
 *
 * In OpenSSL versions prior to 1.1.0, ASN1_TIME_compare() is not available, so we use ASN1_TIME_diff() instead,
 * and may throw an exception if it fails (only when the passed ASN1_TIME values have invalid time format). In all
 * other OpenSSL versions, ASN1_TIME_compare() will be used.
 *
 * @param t1 The first time value.
 * @param t2 The second time value.
 *
 * @returns -1 if t1 < t2, 0 if t1 == t2, 1 if t1 > t2.
 */
int Asn1TimeCompare(const ASN1_TIME* t1, const ASN1_TIME* t2)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	return ASN1_TIME_compare(t1, t2);
#else
	int day, sec;
	if (!ASN1_TIME_diff(&day, &sec, t1, t2)) {
		char errbuf[256];
		ERR_error_string_n(ERR_peek_error(), errbuf, sizeof errbuf);
		Log(LogCritical, "SSL") << "Error on ASN1_TIME_diff: " << ERR_peek_error() << ", \"" << errbuf << "\"";

		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("ASN1_TIME_diff")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	if (day < 0 || sec < 0) {
		return 1; // t1 > t2 (meaning t1 is later than t2)
	}

	if (day > 0 || sec > 0) {
		return -1; // t1 < t2 (meaning t1 is earlier than t2)
	}

	return 0; // t1 == t2
#endif
}

String CertificateToString(X509* cert)
{
	BIO *mem = BIO_new(BIO_s_mem());
	PEM_write_bio_X509(mem, cert);

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

	return BinaryToHex(digest, SHA_DIGEST_LENGTH);
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

String BinaryToHex(const unsigned char* data, size_t length) {
	static const char hexdigits[] = "0123456789abcdef";

	String output(2*length, 0);
	for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
		output[2 * i] = hexdigits[data[i] >> 4];
		output[2 * i + 1] = hexdigits[data[i] & 0xf];
	}

	return output;
}

bool VerifyCertificate(const std::shared_ptr<X509> &caCertificate, const std::shared_ptr<X509> &certificate, const String& crlFile)
{
	return VerifyCertificate(caCertificate.get(), certificate.get(), crlFile);
}

bool VerifyCertificate(X509* caCertificate, X509* certificate, const String& crlFile)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	/*
	 * OpenSSL older than version 1.1.0 stored a valid flag in the struct behind X509* which leads to certain validation
	 * steps to be skipped on subsequent verification operations. If a certificate is verified multiple times with a
	 * different configuration, for example with different trust anchors, this can result in the certificate
	 * incorrectly being treated as valid.
	 *
	 * This issue is worked around by serializing and deserializing the certificate which creates a new struct instance
	 * with the valid flag cleared, hence performing the full validation.
	 *
	 * The flag in question was removed in OpenSSL 1.1.0, so this extra step isn't necessary for more recent versions:
	 * https://github.com/openssl/openssl/commit/0e76014e584ba78ef1d6ecb4572391ef61c4fb51
	 */
	std::shared_ptr<X509> copy = StringToCertificate(CertificateToString(certificate));
	VERIFY(copy.get() != certificate);
	certificate = copy.get();
#endif

	std::unique_ptr<X509_STORE, decltype(&X509_STORE_free)> store{X509_STORE_new(), &X509_STORE_free};

	if (!store)
		return false;

	X509_STORE_add_cert(store.get(), caCertificate);

	if (!crlFile.IsEmpty()) {
		AddCRLToSSLContext(store.get(), crlFile);
	}

	std::unique_ptr<X509_STORE_CTX, decltype(&X509_STORE_CTX_free)> csc{X509_STORE_CTX_new(), &X509_STORE_CTX_free};
	X509_STORE_CTX_init(csc.get(), store.get(), certificate, nullptr);

	int rc = X509_verify_cert(csc.get());

	if (rc == 0) {
		int err = X509_STORE_CTX_get_error(csc.get());

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
	int sign_alg;

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
