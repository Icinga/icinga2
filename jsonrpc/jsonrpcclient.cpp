#include "i2-jsonrpc.h"

using namespace icinga;

void JsonRpcClient::Start(void)
{
	TCPClient::Start();

	OnDataAvailable.bind(bind_weak(&JsonRpcClient::DataAvailableHandler, shared_from_this()));
}

void JsonRpcClient::SendMessage(JsonRpcMessage::RefType message)
{
	Netstring::RefType ns = message->ToNetstring();
	ns->WriteToFIFO(GetSendQueue());
}

int JsonRpcClient::DataAvailableHandler(EventArgs::RefType ea)
{
	Netstring::RefType ns;

	while (true) {
		try {
			ns = Netstring::ReadFromFIFO(GetRecvQueue());
		} catch (const exception&) {
			Close();

			return 1;
		}
	
		if (ns == NULL)
			break;

		JsonRpcMessage::RefType msg = JsonRpcMessage::FromNetstring(ns);
		NewMessageEventArgs::RefType nea = new_object<NewMessageEventArgs>();
		nea->Source = shared_from_this();
		nea->Message = msg;
		OnNewMessage(nea);
	}

	return 0;
}
