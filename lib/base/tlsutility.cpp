/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/tlsutility.hpp"
#include "base/convert.hpp"
#include "base/logger_fwd.hpp"
#include "base/context.hpp"


namespace icinga
{

static bool l_SSLInitialized = false;
static boost::mutex *l_Mutexes;

static void OpenSSLLockingCallback(int mode, int type, const char *, int)
{
	if (mode & CRYPTO_LOCK)
		l_Mutexes[type].lock();
	else
		l_Mutexes[type].unlock();
}

static unsigned long OpenSSLIDCallback(void)
{
#ifdef _WIN32
	return reinterpret_cast<unsigned long>(GetCurrentThreadId());
#else /* _WIN32 */
	return (unsigned long)pthread_self();
#endif /* _WIN32 */
}

/**
 * Initializes the OpenSSL library.
 */
static void InitializeOpenSSL(void)
{
	if (l_SSLInitialized)
		return;

	SSL_library_init();
	SSL_load_error_strings();

	SSL_COMP_get_compression_methods();

	l_Mutexes = new boost::mutex[CRYPTO_num_locks()];
	CRYPTO_set_locking_callback(&OpenSSLLockingCallback);
	CRYPTO_set_id_callback(&OpenSSLIDCallback);

	l_SSLInitialized = true;
}

/**
 * Initializes an SSL context using the specified certificates.
 *
 * @param pubkey The public key.
 * @param privkey The matching private key.
 * @param cakey CA certificate chain file.
 * @returns An SSL context.
 */
shared_ptr<SSL_CTX> MakeSSLContext(const String& pubkey, const String& privkey, const String& cakey)
{
	std::ostringstream msgbuf;
	char errbuf[120];

	InitializeOpenSSL();

	shared_ptr<SSL_CTX> sslContext = shared_ptr<SSL_CTX>(SSL_CTX_new(TLSv1_method()), SSL_CTX_free);

	SSL_CTX_set_mode(sslContext.get(), SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	if (!SSL_CTX_use_certificate_chain_file(sslContext.get(), pubkey.CStr())) {
		msgbuf << "Error with public key file '" << pubkey << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_certificate_chain_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pubkey));
	}

	if (!SSL_CTX_use_PrivateKey_file(sslContext.get(), privkey.CStr(), SSL_FILETYPE_PEM)) {
		msgbuf << "Error with private key file '" << privkey << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_PrivateKey_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(privkey));
	}

	if (!SSL_CTX_check_private_key(sslContext.get())) {
		msgbuf << "Error checking private key '" << privkey << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_check_private_key")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	if (!SSL_CTX_load_verify_locations(sslContext.get(), cakey.CStr(), NULL)) {
		msgbuf << "Error loading and verifying locations in ca key file '" << cakey << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_load_verify_locations")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(cakey));
	}

	STACK_OF(X509_NAME) *cert_names;

	cert_names = SSL_load_client_CA_file(cakey.CStr());
	if (cert_names == NULL) {
		msgbuf << "Error loading client ca key file '" << cakey << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_load_client_CA_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(cakey));
	}

	SSL_CTX_set_client_CA_list(sslContext.get(), cert_names);

	return sslContext;
}

/**
 * Loads a CRL and appends its certificates to the specified SSL context.
 *
 * @param context The SSL context.
 * @param crlPath The path to the CRL file.
 */
void AddCRLToSSLContext(const shared_ptr<SSL_CTX>& context, const String& crlPath)
{
	std::ostringstream msgbuf;
	char errbuf[120];
	X509_STORE *x509_store = SSL_CTX_get_cert_store(context.get());

	X509_LOOKUP *lookup;
	lookup = X509_STORE_add_lookup(x509_store, X509_LOOKUP_file());

	if (!lookup) {
		msgbuf << "Error adding X509 store lookup: " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("X509_STORE_add_lookup")
			<< errinfo_openssl_error(ERR_get_error()));
	}

	if (X509_LOOKUP_load_file(lookup, crlPath.CStr(), X509_FILETYPE_PEM) != 0) {
		msgbuf << "Error loading crl file '" << crlPath << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("X509_LOOKUP_load_file")
			<< errinfo_openssl_error(ERR_get_error())
			<< boost::errinfo_file_name(crlPath));
	}

	X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
	X509_STORE_set1_param(x509_store, param);
	X509_VERIFY_PARAM_free(param);
}

/**
 * Retrieves the common name for an X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
String GetCertificateCN(const shared_ptr<X509>& certificate)
{
	std::ostringstream msgbuf;
	char errbuf[120];
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()),
	    NID_commonName, buffer, sizeof(buffer));

	if (rc == -1) {
		msgbuf << "Error with x509 NAME getting text by NID: " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("X509_NAME_get_text_by_NID")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	return buffer;
}

/**
 * Retrieves an X509 certificate from the specified file.
 *
 * @param pemfile The filename.
 * @returns An X509 certificate.
 */
shared_ptr<X509> GetX509Certificate(const String& pemfile)
{
	std::ostringstream msgbuf;
	char errbuf[120];
	X509 *cert;
	BIO *fpcert = BIO_new(BIO_s_file());

	if (fpcert == NULL) {
		msgbuf << "Error creating new BIO: " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_new")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	if (BIO_read_filename(fpcert, pemfile.CStr()) < 0) {
		msgbuf << "Error reading pem file '" << pemfile << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_read_filename")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pemfile));
	}

	cert = PEM_read_bio_X509_AUX(fpcert, NULL, NULL, NULL);
	if (cert == NULL) {
		msgbuf << "Error on bio X509 AUX reading pem file '" << pemfile << "': " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("PEM_read_bio_X509_AUX")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pemfile));
	}

	BIO_free(fpcert);

	return shared_ptr<X509>(cert, X509_free);
}

int MakeX509CSR(const char *cn, const char *keyfile, const char *csrfile)
{
	InitializeOpenSSL();

	RSA *rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL);

	BIO *bio = BIO_new(BIO_s_file());
	BIO_write_filename(bio, const_cast<char *>(keyfile));
	PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL);
	BIO_free(bio);

	X509_REQ *req = X509_REQ_new();

	if (!req)
		return 0;

	EVP_PKEY *key = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(key, rsa);
	X509_REQ_set_version(req, 0);
	X509_REQ_set_pubkey(req, key);

	X509_NAME *name = X509_REQ_get_subject_name(req);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cn, -1, -1, 0);

	X509_REQ_sign(req, key, EVP_sha1());

	EVP_PKEY_free(key);

	bio = BIO_new(BIO_s_file());
	BIO_write_filename(bio, const_cast<char *>(csrfile));
	PEM_write_bio_X509_REQ(bio, req);
	BIO_free(bio);

	X509_REQ_free(req);

	return 1;
}

String SHA256(const String& s)
{
	std::ostringstream msgbuf;
	char errbuf[120];
	SHA256_CTX context;
	unsigned char digest[SHA256_DIGEST_LENGTH];

	if (!SHA256_Init(&context)) {
		msgbuf << "Error on SHA256 Init: " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Init")
			<< errinfo_openssl_error(ERR_get_error()));
	}

	if (!SHA256_Update(&context, (unsigned char*)s.CStr(), s.GetLength())) {
		msgbuf << "Error on SHA256 Update: " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Update")
			<< errinfo_openssl_error(ERR_get_error()));
	}

	if (!SHA256_Final(digest, &context)) {
		msgbuf << "Error on SHA256 Final: " << ERR_get_error() << ", \"" << ERR_error_string(ERR_get_error(), errbuf) << "\"";
		Log(LogCritical, "SSL", msgbuf.str());
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Final")
			<< errinfo_openssl_error(ERR_get_error()));
	}

	int i;
	char output[SHA256_DIGEST_LENGTH*2+1];
	for (i = 0; i < 32; i++)
		sprintf(output + 2 * i, "%02x", digest[i]);

	return output;
}

}
