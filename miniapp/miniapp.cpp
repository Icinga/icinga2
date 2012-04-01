#include <cstdio>
#include <iostream>
#include <i2-base.h>
#include <i2-jsonrpc.h>

using namespace icinga;

using std::cout;
using std::endl;

namespace icinga
{

class MyApplication : public Application
{
private:
	ConfigHive::RefType m_Hive;

public:
	typedef shared_ptr<MyApplication> RefType;
	typedef weak_ptr<MyApplication> WeakRefType;

	int Main(const vector<string>& args)
	{
		LoadComponent("configcomponent");

		m_Hive = new_object<ConfigHive>();
		
		/*ConfigFileSerializer::RefType cfs = new_object<ConfigFileSerializer>();
		cfs->SetFilename("test.conf");
		cfs->Deserialize(m_Hive);

		ConfigObject::RefType netconfig = m_Hive->GetObject("netconfig");

		JsonRpcServer::RefType ts = new_object<JsonRpcServer>();
		ts->MakeSocket();
		ts->Bind(netconfig->GetPropertyInteger("port", 7777));
		ts->Listen();

		ConnectionManager::RefType cm = new_object<ConnectionManager>();
		cm->RegisterMethod("HelloWorld", bind_weak(&MyApplication::HelloWorld, shared_from_this()));
		cm->RegisterServer(ts);

		ConfigRpcServiceModule::RefType rsm = new_object<ConfigRpcServiceModule>();
		rsm->SetHive(m_Hive);
		rsm->SetConfigSource(true);
		cm->RegisterServiceModule(rsm);*/

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

		ConfigObject::RefType co = new_object<ConfigObject>();
		co->SetName("foo");
		co->SetType("moo");
		m_Hive->AddObject(co);

		return 0;
	}
};

}

SET_START_CLASS(icinga::MyApplication);
