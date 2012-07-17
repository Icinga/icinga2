/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"
#include <mmatch.h>

using namespace icinga;

bool I2_EXPORT Utility::m_SSLInitialized = false;

/**
 * Returns a human-readable type name of a type_info object.
 *
 * @param ti A type_info object.
 * @returns The type name of the object.
 */
string Utility::GetTypeName(const type_info& ti)
{
	string klass = ti.name();

#ifdef HAVE_GCC_ABI_DEMANGLE
	int status;
	char *realname = abi::__cxa_demangle(klass.c_str(), 0, 0, &status);

	if (realname != NULL) {
		klass = string(realname);
		free(realname);
	}
#endif /* HAVE_GCC_ABI_DEMANGLE */

	return klass;
}


/**
 * Detaches from the controlling terminal.
 */
void Utility::Daemonize(void) {
#ifndef _WIN32
	pid_t pid;
	int fd;

	pid = fork();
	if (pid < 0)
		throw_exception(PosixException("fork() failed", errno));

	if (pid)
		exit(0);

	fd = open("/dev/null", O_RDWR);

	if (fd < 0)
		throw_exception(PosixException("open() failed", errno));

	if (fd != STDIN_FILENO)
		dup2(fd, STDIN_FILENO);

	if (fd != STDOUT_FILENO)
		dup2(fd, STDOUT_FILENO);

	if (fd != STDERR_FILENO)
		dup2(fd, STDERR_FILENO);

	if (fd > STDERR_FILENO)
		close(fd);

	if (setsid() < 0)
		throw_exception(PosixException("setsid() failed", errno));
#endif
}

/**
 * Initializes the OpenSSL library.
 */
void Utility::InitializeOpenSSL(void)
{
	if (m_SSLInitialized)
		return;

	SSL_library_init();
	SSL_load_error_strings();

	m_SSLInitialized = true;
}

/**
 * Initializes an SSL context using the specified certificates.
 *
 * @param pubkey The public key.
 * @param privkey The matching private key.
 * @param cakey CA certificate chain file.
 * @returns An SSL context.
 */
shared_ptr<SSL_CTX> Utility::MakeSSLContext(string pubkey, string privkey, string cakey)
{
	InitializeOpenSSL();

	shared_ptr<SSL_CTX> sslContext = shared_ptr<SSL_CTX>(SSL_CTX_new(TLSv1_method()), SSL_CTX_free);

	SSL_CTX_set_mode(sslContext.get(), SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	if (!SSL_CTX_use_certificate_chain_file(sslContext.get(), pubkey.c_str()))
		throw_exception(OpenSSLException("Could not load public X509 key file", ERR_get_error()));

	if (!SSL_CTX_use_PrivateKey_file(sslContext.get(), privkey.c_str(), SSL_FILETYPE_PEM))
		throw_exception(OpenSSLException("Could not load private X509 key file", ERR_get_error()));

	if (!SSL_CTX_load_verify_locations(sslContext.get(), cakey.c_str(), NULL))
		throw_exception(OpenSSLException("Could not load public CA key file", ERR_get_error()));

	STACK_OF(X509_NAME) *cert_names;

	cert_names = SSL_load_client_CA_file(cakey.c_str());
	if (cert_names == NULL)
		throw_exception(OpenSSLException("SSL_load_client_CA_file() failed", ERR_get_error()));

	SSL_CTX_set_client_CA_list(sslContext.get(), cert_names);

	return sslContext;
}

/**
 * Retrieves the common name for an X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
string Utility::GetCertificateCN(const shared_ptr<X509>& certificate)
{
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()), NID_commonName, buffer, sizeof(buffer));

	if (rc == -1)
		throw_exception(OpenSSLException("X509 certificate has no CN attribute", ERR_get_error()));

	return buffer;
}

/**
 * Retrieves an X509 certificate from the specified file.
 *
 * @param pemfile The filename.
 * @returns An X509 certificate.
 */
shared_ptr<X509> Utility::GetX509Certificate(string pemfile)
{
	X509 *cert;
	BIO *fpcert = BIO_new(BIO_s_file());

	if (fpcert == NULL)
		throw_exception(OpenSSLException("BIO_new failed", ERR_get_error()));

	if (BIO_read_filename(fpcert, pemfile.c_str()) < 0)
		throw_exception(OpenSSLException("BIO_read_filename failed", ERR_get_error()));

	cert = PEM_read_bio_X509_AUX(fpcert, NULL, NULL, NULL);
	if (cert == NULL)
		throw_exception(OpenSSLException("PEM_read_bio_X509_AUX failed", ERR_get_error()));

	BIO_free(fpcert);

	return shared_ptr<X509>(cert, X509_free);
}

/**
 * Performs wildcard pattern matching.
 *
 * @param pattern The wildcard pattern.
 * @param text The string that should be checked.
 * @returns true if the wildcard pattern matches, false otherwise.
 */
bool Utility::Match(string pattern, string text)
{
	return (match(pattern.c_str(), text.c_str()) == 0);
}

/**
 * Returns the directory component of a path. See dirname(3) for details.
 *
 * @param path The full path.
 * @returns The directory.
 */
string Utility::DirName(const string& path)
{
	char *dir = strdup(path.c_str());
	string result;

	if (dir == NULL)
		throw_exception(bad_alloc());

#ifndef _WIN32
	result = dirname(dir);
#else /* _WIN32 */
	if (!PathRemoveFileSpec(dir)) {
		free(dir);
		throw_exception(Win32Exception("PathRemoveFileSpec() failed", GetLastError()));
	}

	result = dir;
#endif /* _WIN32 */

	free(dir);

	return result;
}

/**
 * Returns the file component of a path. See basename(3) for details.
 *
 * @param path The full path.
 * @returns The filename.
 */
string Utility::BaseName(const string& path)
{
	char *dir = strdup(path.c_str());
	string result;

	if (dir == NULL)
		throw_exception(bad_alloc());

#ifndef _WIN32
	result = basename(dir);
#else /* _WIN32 */
	result = PathFindFileName(dir);
#endif /* _WIN32 */

	free(dir);

	return result;
}
