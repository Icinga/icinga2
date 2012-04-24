#include "i2-base.h"

using namespace icinga;

TLSClient::TLSClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext) : TCPClient(role)
{
	m_SSLContext = sslContext;
}

shared_ptr<X509> TLSClient::GetClientCertificate(void) const
{
	return shared_ptr<X509>(SSL_get_certificate(m_SSL.get()), X509_free);
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
		; /* TODO: deal with error */

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
				/* TODO: deal with error */

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
				/* TODO: deal with error */

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

TCPClient::Ptr icinga::TLSClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	return make_shared<TLSClient>(role, sslContext);
}
