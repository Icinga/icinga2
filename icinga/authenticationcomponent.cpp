#include "i2-icinga.h"

using namespace icinga;

IcingaApplication::Ptr AuthenticationComponent::GetIcingaApplication(void) const
{
	return static_pointer_cast<IcingaApplication>(GetApplication());
}

string AuthenticationComponent::GetName(void) const
{
	return "authenticationcomponent";
}

void AuthenticationComponent::Start(void)
{
	m_AuthenticationEndpoint = make_shared<VirtualEndpoint>();
	m_AuthenticationEndpoint->RegisterMethodHandler("message::SetIdentity", bind_weak(&AuthenticationComponent::IdentityMessageHandler, shared_from_this()));

	EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
	mgr->OnNewEndpoint += bind_weak(&AuthenticationComponent::NewEndpointHandler, shared_from_this());
	mgr->ForeachEndpoint(bind(&AuthenticationComponent::NewEndpointHandler, this, _1));
	mgr->RegisterEndpoint(m_AuthenticationEndpoint);
}

void AuthenticationComponent::Stop(void)
{

}

int AuthenticationComponent::NewEndpointHandler(const NewEndpointEventArgs& neea)
{
	if (neea.Endpoint->IsLocal())
		return 0;

	JsonRpcRequest request;
	request.SetVersion("2.0");
	request.SetMethod("message::SetIdentity");

	IdentityMessage params;
	params.SetIdentity("keks");
	request.SetParams(params);

	neea.Endpoint->ProcessRequest(m_AuthenticationEndpoint, request);
}

int AuthenticationComponent::IdentityMessageHandler(const NewRequestEventArgs& nrea)
{
	Message params;
	if (!nrea.Request.GetParams(&params))
		return 0;

	IdentityMessage identityMessage = params;

	string identity;
	if (!identityMessage.GetIdentity(&identity))
		return 0;

	nrea.Sender->SetIdentity(identity);

	/* there's no authentication for now, just tell them it's ok to send messages */
	JsonRpcRequest request;
	request.SetVersion("2.0");
	request.SetMethod("message::Welcome");
	nrea.Sender->ProcessRequest(m_AuthenticationEndpoint, request);

	return 0;
}
