#include "i2-base.h"

using namespace icinga;

int I2_EXPORT TLSClient::m_SSLIndex;
bool I2_EXPORT TLSClient::m_SSLIndexInitialized = false;

TLSClient::TLSClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext) : TCPClient(role)
{
	m_SSLContext = sslContext;
}

void TLSClient::NullCertificateDeleter(X509 *certificate)
{
	/* Nothing to do here. */
}

shared_ptr<X509> TLSClient::GetClientCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_certificate(m_SSL.get()), &TLSClient::NullCertificateDeleter);
}

shared_ptr<X509> TLSClient::GetPeerCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_peer_certificate(m_SSL.get()), X509_free);
}

void TLSClient::Start(void)
{
	TCPClient::Start();

	m_SSL = shared_ptr<SSL>(SSL_new(m_SSLContext.get()), SSL_free);

	if (!m_SSL)
		throw OpenSSLException("SSL_new failed", ERR_get_error());

	if (!GetClientCertificate())
		throw InvalidArgumentException("No X509 client certificate was specified.");

	if (!m_SSLIndexInitialized) {
		m_SSLIndex = SSL_get_ex_new_index(0, (void *)"TLSClient", NULL, NULL, NULL);
		m_SSLIndexInitialized = true;
	}

	SSL_set_ex_data(m_SSL.get(), m_SSLIndex, this);

	SSL_set_verify(m_SSL.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
	    &TLSClient::SSLVerifyCertificate);

	BIO *bio = BIO_new_socket(GetFD(), 0);
	SSL_set_bio(m_SSL.get(), bio, bio);

	if (GetRole() == RoleInbound)
		SSL_set_accept_state(m_SSL.get());
	else
		SSL_set_connect_state(m_SSL.get());
}

int TLSClient::ReadableEventHandler(const EventArgs& ea)
{
	int rc;

	size_t bufferSize = FIFO::BlockSize / 2;
	char *buffer = (char *)GetRecvQueue()->GetWriteBuffer(&bufferSize);
	rc = SSL_read(m_SSL.get(), buffer, bufferSize);

	if (rc <= 0) {
		switch (SSL_get_error(m_SSL.get(), rc)) {
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_WANT_READ:
				return 0;
			case SSL_ERROR_ZERO_RETURN:
				Close();

				return 0;
			default:
				HandleSSLError();

				return 0;
		}
	}

	GetRecvQueue()->Write(NULL, rc);

	EventArgs dea;
	dea.Source = shared_from_this();
	OnDataAvailable(dea);

	return 0;
}

int TLSClient::WritableEventHandler(const EventArgs& ea)
{
	int rc;

	rc = SSL_write(m_SSL.get(), (const char *)GetSendQueue()->GetReadBuffer(), GetSendQueue()->GetSize());

	if (rc <= 0) {
		switch (SSL_get_error(m_SSL.get(), rc)) {
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_WANT_READ:
				return 0;
			case SSL_ERROR_ZERO_RETURN:
				Close();

				return 0;
			default:
				HandleSSLError();

				return 0;
		}
	}

	GetSendQueue()->Read(NULL, rc);

	return 0;
}

bool TLSClient::WantsToWrite(void) const
{
	if (SSL_want_write(m_SSL.get()))
		return true;

	if (SSL_state(m_SSL.get()) != SSL_ST_OK)
		return false;

	return TCPClient::WantsToWrite();
}

void TLSClient::CloseInternal(bool from_dtor)
{
	SSL_shutdown(m_SSL.get());

	TCPClient::CloseInternal(from_dtor);
}

void TLSClient::HandleSSLError(void)
{
	int code = ERR_get_error();

	if (code != 0) {
		SocketErrorEventArgs sea;
		sea.Code = code;
		sea.Message = OpenSSLException::FormatErrorCode(sea.Code);
		OnError(sea);
	}

	Close();
	return;
}

TCPClient::Ptr icinga::TLSClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	return make_shared<TLSClient>(role, sslContext);
}

int TLSClient::SSLVerifyCertificate(int ok, X509_STORE_CTX *x509Context)
{
	SSL *ssl = (SSL *)X509_STORE_CTX_get_ex_data(x509Context, SSL_get_ex_data_X509_STORE_CTX_idx());
	TLSClient *client = (TLSClient *)SSL_get_ex_data(ssl, m_SSLIndex);

	if (client == NULL)
		return 0;

	VerifyCertificateEventArgs vcea;
	vcea.Source = client->shared_from_this();
	vcea.ValidCertificate = (ok != 0);
	vcea.Context = x509Context;
	vcea.Certificate = shared_ptr<X509>(x509Context->cert, &TLSClient::NullCertificateDeleter);
	client->OnVerifyCertificate(vcea);

	return (int)vcea.ValidCertificate;
}
