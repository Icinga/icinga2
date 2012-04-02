#include "i2-jsonrpc.h"

using namespace icinga;

void JsonRpcClient::Start(void)
{
	TCPClient::Start();

	OnDataAvailable.bind(bind_weak(&JsonRpcClient::DataAvailableHandler, shared_from_this()));
}

void JsonRpcClient::SendMessage(JsonRpcMessage::Ptr message)
{
	Netstring::WriteJSONToFIFO(GetSendQueue(), message->GetJSON());
}

int JsonRpcClient::DataAvailableHandler(EventArgs::Ptr ea)
{
	cJSON *json;

	while (true) {
		try {
			json = Netstring::ReadJSONFromFIFO(GetRecvQueue());
		} catch (const exception&) {
			Close();

			return 1;
		}
	
		if (json == NULL)
			break;

		JsonRpcMessage::Ptr msg = new_object<JsonRpcMessage>();
		msg->SetJSON(json);
		NewMessageEventArgs::Ptr nea = new_object<NewMessageEventArgs>();
		nea->Source = shared_from_this();
		nea->Message = msg;
		OnNewMessage(nea);
	}

	return 0;
}
