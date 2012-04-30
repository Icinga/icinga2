#include "i2-jsonrpc.h"

using namespace icinga;

JsonRpcClient::JsonRpcClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
    : TLSClient(role, sslContext) { }

void JsonRpcClient::Start(void)
{
	TLSClient::Start();

	OnDataAvailable += bind_weak(&JsonRpcClient::DataAvailableHandler, shared_from_this());
}

void JsonRpcClient::SendMessage(const Message& message)
{
	Netstring::WriteMessageToFIFO(GetSendQueue(), message);
}

int JsonRpcClient::DataAvailableHandler(const EventArgs& ea)
{
	while (true) {
		try {
			Message message;

			if (Netstring::ReadMessageFromFIFO(GetRecvQueue(), &message)) {
				NewMessageEventArgs nea;
				nea.Source = shared_from_this();
				nea.Message = message;
				OnNewMessage(nea);
			}
		} catch (const Exception& ex) {
			Application::Log("Exception while processing message from JSON-RPC client: " + ex.GetMessage());
			Close();

			return 1;
		}
	}

	return 0;
}

TCPClient::Ptr icinga::JsonRpcClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	return make_shared<JsonRpcClient>(role, sslContext);
}
