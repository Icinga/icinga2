/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/logger.hpp"
#include "base/context.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include <fstream>

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
	return (unsigned long)GetCurrentThreadId();
#else /* _WIN32 */
	return (unsigned long)pthread_self();
#endif /* _WIN32 */
}

/**
 * Initializes the OpenSSL library.
 */
void InitializeOpenSSL(void)
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
boost::shared_ptr<SSL_CTX> MakeSSLContext(const String& pubkey, const String& privkey, const String& cakey)
{
	char errbuf[120];

	InitializeOpenSSL();

	boost::shared_ptr<SSL_CTX> sslContext = boost::shared_ptr<SSL_CTX>(SSL_CTX_new(TLSv1_method()), SSL_CTX_free);

	SSL_CTX_set_mode(sslContext.get(), SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	SSL_CTX_set_session_id_context(sslContext.get(), (const unsigned char *)"Icinga 2", 8);

	if (!SSL_CTX_use_certificate_chain_file(sslContext.get(), pubkey.CStr())) {
		Log(LogCritical, "SSL")
		    << "Error with public key file '" << pubkey << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_certificate_chain_file")
		    << errinfo_openssl_error(ERR_peek_error())
		    << boost::errinfo_file_name(pubkey));
	}

	if (!SSL_CTX_use_PrivateKey_file(sslContext.get(), privkey.CStr(), SSL_FILETYPE_PEM)) {
		Log(LogCritical, "SSL")
		    << "Error with private key file '" << privkey << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_PrivateKey_file")
		    << errinfo_openssl_error(ERR_peek_error())
		    << boost::errinfo_file_name(privkey));
	}

	if (!SSL_CTX_check_private_key(sslContext.get())) {
		Log(LogCritical, "SSL")
		    << "Error checking private key '" << privkey << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_check_private_key")
		    << errinfo_openssl_error(ERR_peek_error()));
	}

	if (!cakey.IsEmpty()) {
		if (!SSL_CTX_load_verify_locations(sslContext.get(), cakey.CStr(), NULL)) {
			Log(LogCritical, "SSL")
			    << "Error loading and verifying locations in ca key file '" << cakey << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
			    << boost::errinfo_api_function("SSL_CTX_load_verify_locations")
			    << errinfo_openssl_error(ERR_peek_error())
			    << boost::errinfo_file_name(cakey));
		}

		STACK_OF(X509_NAME) *cert_names;

		cert_names = SSL_load_client_CA_file(cakey.CStr());
		if (cert_names == NULL) {
			Log(LogCritical, "SSL")
			    << "Error loading client ca key file '" << cakey << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
			    << boost::errinfo_api_function("SSL_load_client_CA_file")
			    << errinfo_openssl_error(ERR_peek_error())
			    << boost::errinfo_file_name(cakey));
		}

		SSL_CTX_set_client_CA_list(sslContext.get(), cert_names);
	}

	return sslContext;
}

/**
 * Loads a CRL and appends its certificates to the specified SSL context.
 *
 * @param context The SSL context.
 * @param crlPath The path to the CRL file.
 */
void AddCRLToSSLContext(const boost::shared_ptr<SSL_CTX>& context, const String& crlPath)
{
	char errbuf[120];
	X509_STORE *x509_store = SSL_CTX_get_cert_store(context.get());

	X509_LOOKUP *lookup;
	lookup = X509_STORE_add_lookup(x509_store, X509_LOOKUP_file());

	if (!lookup) {
		Log(LogCritical, "SSL")
		    << "Error adding X509 store lookup: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("X509_STORE_add_lookup")
		    << errinfo_openssl_error(ERR_peek_error()));
	}

	if (X509_LOOKUP_load_file(lookup, crlPath.CStr(), X509_FILETYPE_PEM) != 0) {
		Log(LogCritical, "SSL")
		    << "Error loading crl file '" << crlPath << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
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

/**
 * Retrieves the common name for an X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
String GetCertificateCN(const boost::shared_ptr<X509>& certificate)
{
	char errbuf[120];
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()),
	    NID_commonName, buffer, sizeof(buffer));

	if (rc == -1) {
		Log(LogCritical, "SSL")
		    << "Error with x509 NAME getting text by NID: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("X509_NAME_get_text_by_NID")
		    << errinfo_openssl_error(ERR_peek_error()));
	}

	return buffer;
}

/**
 * Retrieves an X509 certificate from the specified file.
 *
 * @param pemfile The filename.
 * @returns An X509 certificate.
 */
boost::shared_ptr<X509> GetX509Certificate(const String& pemfile)
{
	char errbuf[120];
	X509 *cert;
	BIO *fpcert = BIO_new(BIO_s_file());

	if (fpcert == NULL) {
		Log(LogCritical, "SSL")
		    << "Error creating new BIO: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_new")
		    << errinfo_openssl_error(ERR_peek_error()));
	}

	if (BIO_read_filename(fpcert, pemfile.CStr()) < 0) {
		Log(LogCritical, "SSL")
		    << "Error reading pem file '" << pemfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_read_filename")
		    << errinfo_openssl_error(ERR_peek_error())
		    << boost::errinfo_file_name(pemfile));
	}

	cert = PEM_read_bio_X509_AUX(fpcert, NULL, NULL, NULL);
	if (cert == NULL) {
		Log(LogCritical, "SSL")
		    << "Error on bio X509 AUX reading pem file '" << pemfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("PEM_read_bio_X509_AUX")
		    << errinfo_openssl_error(ERR_peek_error())
		    << boost::errinfo_file_name(pemfile));
	}

	BIO_free(fpcert);

	return boost::shared_ptr<X509>(cert, X509_free);
}

int MakeX509CSR(const String& cn, const String& keyfile, const String& csrfile, const String& certfile, bool ca)
{
	char errbuf[120];

	InitializeOpenSSL();

	RSA *rsa = RSA_generate_key(4096, RSA_F4, NULL, NULL);

	Log(LogInformation, "base")
	    << "Writing private key to '" << keyfile << "'.";

	BIO *bio = BIO_new_file(const_cast<char *>(keyfile.CStr()), "w");

	if (!bio) {
		Log(LogCritical, "SSL")
		    << "Error while opening private RSA key file '" << keyfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_new_file")
		    << errinfo_openssl_error(ERR_peek_error())
		    << boost::errinfo_file_name(keyfile));
	}

	if (!PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL)) {
		Log(LogCritical, "SSL")
		    << "Error while writing private RSA key to file '" << keyfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
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

		boost::shared_ptr<X509> cert = CreateCert(key, subject, subject, key, ca);

		X509_NAME_free(subject);

		Log(LogInformation, "base")
		    << "Writing X509 certificate to '" << certfile << "'.";

		bio = BIO_new_file(const_cast<char *>(certfile.CStr()), "w");

		if (!bio) {
			Log(LogCritical, "SSL")
			    << "Error while opening certificate file '" << certfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
			    << boost::errinfo_api_function("BIO_new_file")
			    << errinfo_openssl_error(ERR_peek_error())
			    << boost::errinfo_file_name(certfile));
		}

		if (!PEM_write_bio_X509(bio, cert.get())) {
			Log(LogCritical, "SSL")
			    << "Error while writing certificate to file '" << certfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
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
	
		X509_REQ_sign(req, key, EVP_sha256());
	
		Log(LogInformation, "base")
		    << "Writing certificate signing request to '" << csrfile << "'.";
	
		bio = BIO_new_file(const_cast<char *>(csrfile.CStr()), "w");

		if (!bio) {
			Log(LogCritical, "SSL")
			    << "Error while opening CSR file '" << csrfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
			BOOST_THROW_EXCEPTION(openssl_error()
			    << boost::errinfo_api_function("BIO_new_file")
			    << errinfo_openssl_error(ERR_peek_error())
			    << boost::errinfo_file_name(csrfile));
		}

		if (!PEM_write_bio_X509_REQ(bio, req)) {
			Log(LogCritical, "SSL")
			    << "Error while writing CSR to file '" << csrfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
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

boost::shared_ptr<X509> CreateCert(EVP_PKEY *pubkey, X509_NAME *subject, X509_NAME *issuer, EVP_PKEY *cakey, bool ca, const String& serialfile)
{
	X509 *cert = X509_new();
	X509_gmtime_adj(X509_get_notBefore(cert), 0);
	X509_gmtime_adj(X509_get_notAfter(cert), 365 * 24 * 60 * 60 * 30);
	X509_set_pubkey(cert, pubkey);

	X509_set_subject_name(cert, subject);
	X509_set_issuer_name(cert, issuer);

	if (!serialfile.IsEmpty()) {
		int serial = 0;

		std::ifstream ifp;
		ifp.open(serialfile.CStr());
		ifp >> std::hex >> serial;
		ifp.close();

		if (ifp.fail())
			BOOST_THROW_EXCEPTION(std::runtime_error("Could not read serial file."));

		std::ofstream ofp;
		ofp.open(serialfile.CStr());
		ofp << std::hex << serial + 1;
		ofp.close();

		if (ofp.fail())
			BOOST_THROW_EXCEPTION(std::runtime_error("Could not update serial file."));

		ASN1_INTEGER_set(X509_get_serialNumber(cert), serial);
	}

	if (ca) {
		X509_EXTENSION *ext;
		X509V3_CTX ctx;
		X509V3_set_ctx_nodb(&ctx);
		X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
		ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_basic_constraints, const_cast<char *>("critical,CA:TRUE"));

		if (ext)
			X509_add_ext(cert, ext, -1);

		X509_EXTENSION_free(ext);
	}

	X509_sign(cert, cakey, EVP_sha256());

	return boost::shared_ptr<X509>(cert, X509_free);
}

String GetIcingaCADir(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/ca";
}

boost::shared_ptr<X509> CreateCertIcingaCA(EVP_PKEY *pubkey, X509_NAME *subject)
{
	char errbuf[120];

	String cadir = GetIcingaCADir();

	String cakeyfile = cadir + "/ca.key";

	RSA *rsa;

	BIO *cakeybio = BIO_new_file(const_cast<char *>(cakeyfile.CStr()), "r");

	if (!cakeybio) {
		Log(LogCritical, "SSL")
		    << "Could not open CA key file '" << cakeyfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		return boost::shared_ptr<X509>();
	}

	rsa = PEM_read_bio_RSAPrivateKey(cakeybio, NULL, NULL, NULL);

	if (!rsa) {
		Log(LogCritical, "SSL")
		    << "Could not read RSA key from CA key file '" << cakeyfile << "': " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		return boost::shared_ptr<X509>();
	}

	BIO_free(cakeybio);

	String cacertfile = cadir + "/ca.crt";

	boost::shared_ptr<X509> cacert = GetX509Certificate(cacertfile);

	EVP_PKEY *privkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(privkey, rsa);

	return CreateCert(pubkey, subject, X509_get_subject_name(cacert.get()), privkey, false, cadir + "/serial.txt");
}

String CertificateToString(const boost::shared_ptr<X509>& cert)
{
	BIO *mem = BIO_new(BIO_s_mem());
	PEM_write_bio_X509(mem, cert.get());

	char *data;
	long len = BIO_get_mem_data(mem, &data);

	String result = String(data, data + len);

	BIO_free(mem);

	return result;
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

String SHA256(const String& s)
{
	char errbuf[120];
	SHA256_CTX context;
	unsigned char digest[SHA256_DIGEST_LENGTH];

	if (!SHA256_Init(&context)) {
		Log(LogCritical, "SSL")
		    << "Error on SHA256 Init: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SHA256_Init")
		    << errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA256_Update(&context, (unsigned char*)s.CStr(), s.GetLength())) {
		Log(LogCritical, "SSL")
		    << "Error on SHA256 Update: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SHA256_Update")
		    << errinfo_openssl_error(ERR_peek_error()));
	}

	if (!SHA256_Final(digest, &context)) {
		Log(LogCritical, "SSL")
		    << "Error on SHA256 Final: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
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
	unsigned char *bytes = new unsigned char[length];

	if (!RAND_bytes(bytes, length)) {
		delete [] bytes;

		char errbuf[120];

		Log(LogCritical, "SSL")
			<< "Error for RAND_bytes: " << ERR_peek_error() << ", \"" << ERR_error_string(ERR_peek_error(), errbuf) << "\"";
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("RAND_bytes")
			<< errinfo_openssl_error(ERR_peek_error()));
	}

	char *output = new char[length * 2 + 1];
	for (int i = 0; i < length; i++)
		sprintf(output + 2 * i, "%02x", bytes[i]);

	String result = output;
	delete [] output;

	return result;
}

}
