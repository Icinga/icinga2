#include "i2-jsonrpc.h"

using namespace icinga;

void JsonRpcClient::Start(void)
{
	TCPClient::Start();

	OnDataAvailable += bind_weak(&JsonRpcClient::DataAvailableHandler, shared_from_this());
}

void JsonRpcClient::SendMessage(Message::Ptr message)
{
	Netstring::WriteMessageToFIFO(GetSendQueue(), message);
}

int JsonRpcClient::DataAvailableHandler(EventArgs::Ptr ea)
{
	Message::Ptr message;

	while (true) {
		try {
			message = Netstring::ReadMessageFromFIFO(GetRecvQueue());
		} catch (const exception&) {
			Close();

			return 1;
		}
	
		if (message.get() == NULL)
			break;

		NewMessageEventArgs::Ptr nea = make_shared<NewMessageEventArgs>();
		nea->Source = shared_from_this();
		nea->Message = message;
		OnNewMessage(nea);
	}

	return 0;
}
