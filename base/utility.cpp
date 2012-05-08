#include "i2-base.h"

using namespace icinga;

bool I2_EXPORT Utility::m_SSLInitialized = false;

/**
 * Daemonize
 *
 * Detaches from the controlling terminal.
 */
void Utility::Daemonize(void) {
#ifndef _WIN32
	pid_t pid;
	int fd;

	pid = fork();
	if (pid < 0)
		throw PosixException("fork failed", errno);

	if (pid)
		exit(0);

	fd = open("/dev/null", O_RDWR);

	if (fd < 0)
		throw PosixException("open failed", errno);

	if (fd != 0)
		dup2(fd, 0);

	if (fd != 1)
		dup2(fd, 1);

	if (fd != 2)
		dup2(fd, 2);

	if (fd > 2)
		close(fd);

	if (setsid() < 0)
		throw PosixException("setsid failed", errno);
#endif
}

/**
 * InitializeOpenSSL
 *
 * Initializes the OpenSSL library.
 */
void Utility::InitializeOpenSSL(void)
{
	if (!m_SSLInitialized) {
		SSL_library_init();
		SSL_load_error_strings();

		m_SSLInitialized = true;
	}
}

/**
 * MakeSSLContext
 *
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

	SSL_METHOD *sslMethod = (SSL_METHOD *)TLSv1_method();

	shared_ptr<SSL_CTX> sslContext = shared_ptr<SSL_CTX>(SSL_CTX_new(sslMethod), SSL_CTX_free);

	SSL_CTX_set_mode(sslContext.get(), SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	if (!SSL_CTX_use_certificate_chain_file(sslContext.get(), pubkey.c_str()))
		throw InvalidArgumentException("Could not load public X509 key file.");

	if (!SSL_CTX_use_PrivateKey_file(sslContext.get(), privkey.c_str(), SSL_FILETYPE_PEM))
		throw InvalidArgumentException("Could not load private X509 key file.");

	if (!SSL_CTX_load_verify_locations(sslContext.get(), cakey.c_str(), NULL))
		throw InvalidArgumentException("Could not load public CA key file.");

	return sslContext;
}

/**
 * GetCertificateCN
 *
 * Retrieves the common name for a X509 certificate.
 *
 * @param certificate The X509 certificate.
 * @returns The common name.
 */
string Utility::GetCertificateCN(const shared_ptr<X509>& certificate)
{
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()), NID_commonName, buffer, sizeof(buffer));

	if (rc == -1)
		throw InvalidArgumentException("X509 certificate has no CN attribute.");

	return buffer;
}

/**
 * GetX509Certificate
 *
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
		throw OpenSSLException("BIO_new failed", ERR_get_error());

	if (BIO_read_filename(fpcert, pemfile.c_str()) < 0)
		throw OpenSSLException("BIO_read_filename failed", ERR_get_error());

	cert = PEM_read_bio_X509_AUX(fpcert, NULL, NULL, NULL);
	if (cert == NULL)
		throw OpenSSLException("PEM_read_bio_X509_AUX failed", ERR_get_error());

	BIO_free(fpcert);

	return shared_ptr<X509>(cert, X509_free);
}
