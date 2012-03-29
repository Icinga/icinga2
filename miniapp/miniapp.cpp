#include <cstdio>
#include <iostream>
#include <i2-base.h>
#include <i2-jsonrpc.h>

using namespace icinga;

using std::cout;
using std::endl;

class MyApplication : public Application
{
public:
	typedef shared_ptr<MyApplication> RefType;
	typedef weak_ptr<MyApplication> WeakRefType;

	int Main(const vector<string>& args)
	{
		JsonRpcServer::RefType ts = new_object<JsonRpcServer>();
		ts->MakeSocket();
		ts->Bind(7777);
		ts->Listen();

		ConnectionManager::RefType cm = new_object<ConnectionManager>();
		cm->RegisterMethod("HelloWorld", bind_weak(&MyApplication::HelloWorld, shared_from_this()));
		cm->BindServer(ts);

		RunEventLoop();

		return 0;
	}

	int HelloWorld(NewMessageEventArgs::RefType nea)
	{
		JsonRpcClient::RefType client = static_pointer_cast<JsonRpcClient>(nea->Source);
		JsonRpcMessage::RefType msg = nea->Message;

		JsonRpcMessage::RefType response = new_object<JsonRpcMessage>();
		response->SetVersion("2.0");
		response->SetID(msg->GetID());
		cJSON *result = response->GetResult();
		cJSON_AddStringToObject(result, "greeting", "Hello World!");
		client->SendMessage(response);
		
		return 0;
	}
};

SET_START_CLASS(MyApplication);
