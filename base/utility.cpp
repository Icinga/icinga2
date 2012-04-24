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

void Utility::InitializeOpenSSL(void)
{
	if (!m_SSLInitialized) {
		SSL_library_init();
		SSL_load_error_strings();

		m_SSLInitialized = true;
	}
}

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

string Utility::GetCertificateCN(const shared_ptr<X509>& certificate)
{
	char buffer[256];

	int rc = X509_NAME_get_text_by_NID(X509_get_subject_name(certificate.get()), NID_commonName, buffer, sizeof(buffer));

	if (rc == -1)
		throw InvalidArgumentException("X509 certificate has no CN attribute.");

	return buffer;
}
