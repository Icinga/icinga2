#include <cstdio>
#include <iostream>
#include <i2-base.h>
#include <i2-jsonrpc.h>

using namespace icinga;

using std::cout;
using std::endl;

class MyApplication : public Application
{
private:
	int m_Foo;

public:
	typedef shared_ptr<MyApplication> RefType;
	typedef weak_ptr<MyApplication> WeakRefType;

	MyApplication(void)
	{
		m_Foo = 0;
	}

	int Main(const vector<string>& args)
	{
		/*FIFO::RefType f = new_object<FIFO>();
		f->Write("12:Hello World!,", 16);
		Netstring::RefType ns = new_object<Netstring>();
		ns->ReadFromFIFO(f);

		Timer::RefType t = new_object<Timer>();
		t->SetInterval(2);
		t->OnTimerExpired.bind(bind_weak(&MyApplication::TimerCallback, shared_from_this()));
		t->Start();*/

		JsonRpcServer::RefType ts = new_object<JsonRpcServer>();
		ts->MakeSocket();
		ts->Bind(9999);
		ts->Listen();

		ConnectionManager::RefType cm = new_object<ConnectionManager>();
		cm->OnNewMessage.bind(bind_weak(&MyApplication::MessageHandler, shared_from_this()));
		cm->BindServer(ts);

		RunEventLoop();

		return 0;
	}

	int MessageHandler(NewMessageEventArgs::RefType nea)
	{
		JsonRpcClient::RefType client = static_pointer_cast<JsonRpcClient>(nea->Source);
		JsonRpcMessage::RefType msg = nea->Message;

		JsonRpcMessage::RefType response = new_object<JsonRpcMessage>();
		response->SetVersion("2.0");
		response->SetID(msg->GetID());
		response->SetResult("moo");
		client->SendMessage(response);
		
		return 0;
	}

	int TimerCallback(TimerEventArgs::RefType tda)
	{
		Timer::RefType t = static_pointer_cast<Timer>(tda->Source);

		m_Foo++;

		printf("Hello World!\n");
		
		if (m_Foo >= 5) {
			t->Stop();
			Shutdown();
		}

		return 0;
	}
};

SET_START_CLASS(MyApplication);
