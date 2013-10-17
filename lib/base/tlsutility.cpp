/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/tlsutility.h"

namespace icinga
{

static bool l_SSLInitialized = false;

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
	InitializeOpenSSL();

	shared_ptr<SSL_CTX> sslContext = shared_ptr<SSL_CTX>(SSL_CTX_new(TLSv1_method()), SSL_CTX_free);

	SSL_CTX_set_mode(sslContext.get(), 0);

	if (!SSL_CTX_use_certificate_chain_file(sslContext.get(), pubkey.CStr())) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_certificate_chain_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pubkey));
	}

	if (!SSL_CTX_use_PrivateKey_file(sslContext.get(), privkey.CStr(), SSL_FILETYPE_PEM)) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_use_PrivateKey_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(privkey));
	}

	if (!SSL_CTX_check_private_key(sslContext.get())) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_check_private_key")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	if (!SSL_CTX_load_verify_locations(sslContext.get(), cakey.CStr(), NULL)) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_CTX_load_verify_locations")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(cakey));
	}

	STACK_OF(X509_NAME) *cert_names;

	cert_names = SSL_load_client_CA_file(cakey.CStr());
	if (cert_names == NULL) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("SSL_load_client_CA_file")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(cakey));
	}

	SSL_CTX_set_client_CA_list(sslContext.get(), cert_names);

	return sslContext;
}

/**
 * Retrieves the common name for an X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
String GetCertificateCN(const shared_ptr<X509>& certificate)
{
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()),
	    NID_commonName, buffer, sizeof(buffer));

	if (rc == -1) {
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
	X509 *cert;
	BIO *fpcert = BIO_new(BIO_s_file());

	if (fpcert == NULL) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_new")
		    << errinfo_openssl_error(ERR_get_error()));
	}

	if (BIO_read_filename(fpcert, pemfile.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("BIO_read_filename")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pemfile));
	}

	cert = PEM_read_bio_X509_AUX(fpcert, NULL, NULL, NULL);
	if (cert == NULL) {
		BOOST_THROW_EXCEPTION(openssl_error()
		    << boost::errinfo_api_function("PEM_read_bio_X509_AUX")
		    << errinfo_openssl_error(ERR_get_error())
		    << boost::errinfo_file_name(pemfile));
	}

	BIO_free(fpcert);

	return shared_ptr<X509>(cert, X509_free);
}

String SHA256(const String& s)
{
	SHA256_CTX context;
	unsigned char digest[SHA256_DIGEST_LENGTH];

	if (!SHA256_Init(&context)) {
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Init")
			<< errinfo_openssl_error(ERR_get_error()));
	}

	if (!SHA256_Update(&context, (unsigned char*)s.CStr(), s.GetLength())) {
		BOOST_THROW_EXCEPTION(openssl_error()
			<< boost::errinfo_api_function("SHA256_Update")
			<< errinfo_openssl_error(ERR_get_error()));
	}

	if (!SHA256_Final(digest, &context)) {
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
