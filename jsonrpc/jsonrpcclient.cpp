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
	Message message;
	bool message_read;

	while (true) {
		try {
			message_read = Netstring::ReadMessageFromFIFO(GetRecvQueue(), &message);
		} catch (const Exception& ex) {
			cerr << "Exception while reading from JSON-RPC client: " << ex.GetMessage() << endl;
			Close();

			return 1;
		}
	
		if (!message_read)
			break;

		NewMessageEventArgs nea;
		nea.Source = shared_from_this();
		nea.Message = message;
		OnNewMessage(nea);
	}

	return 0;
}

TCPClient::Ptr icinga::JsonRpcClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext)
{
	return make_shared<JsonRpcClient>(role, sslContext);
}
