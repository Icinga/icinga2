#include "i2-demo.h"

using namespace icinga;

IcingaApplication::Ptr DemoComponent::GetIcingaApplication(void)
{
	return static_pointer_cast<IcingaApplication>(GetApplication());
}

string DemoComponent::GetName(void) const
{
	return "democomponent";
}

void DemoComponent::Start(void)
{
	m_DemoEndpoint = make_shared<VirtualEndpoint>();
	m_DemoEndpoint->RegisterMethodSink("demo::HelloWorld");
	m_DemoEndpoint->RegisterMethodSource("demo::HelloWorld");

	EndpointManager::Ptr endpointManager = GetIcingaApplication()->GetEndpointManager();
	endpointManager->RegisterEndpoint(m_DemoEndpoint);

	endpointManager->OnNewEndpoint += bind_weak(&DemoComponent::NewEndpointHandler, shared_from_this());
	endpointManager->ForeachEndpoint(bind(&DemoComponent::NewEndpointHandler, this, _1));

	m_DemoTimer = make_shared<Timer>();
	m_DemoTimer->SetInterval(1);
	m_DemoTimer->OnTimerExpired += bind_weak(&DemoComponent::DemoTimerHandler, shared_from_this());
	m_DemoTimer->Start();
}

void DemoComponent::Stop(void)
{
	EndpointManager::Ptr endpointManager = GetIcingaApplication()->GetEndpointManager();
	endpointManager->UnregisterEndpoint(m_DemoEndpoint);
}

int AuthenticationComponent::NewEndpointHandler(const NewEndpointEventArgs& neea)
{
	neea.Endpoint->AddAllowedMethodSinkPrefix("demo");
	neea.Endpoint->AddAllowedMethodSourcePrefix("demo");

	return 0;
}

int DemoComponent::DemoTimerHandler(const TimerEventArgs& tea)
{
	cout << "Sending multicast 'hello world' message." << endl;

	JsonRpcRequest request;
	request.SetMethod("test");

	EndpointManager::Ptr endpointManager = GetIcingaApplication()->GetEndpointManager();

	for (int i = 0; i < 5; i++)
		endpointManager->SendMulticastRequest(m_DemoEndpoint, request);

	return 0;
}

int DemoComponent::HelloWorldRequestHAndler(const NewRequestEventArgs& nrea)
{
	cout << "Got Hello World from " << nrea.Sender->GetAddress();

	return 0;
}

EXPORT_COMPONENT(DemoComponent);
