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
	m_AuthenticationEndpoint->RegisterMethodHandler("auth::SetIdentity", bind_weak(&AuthenticationComponent::IdentityMessageHandler, shared_from_this()));

	EndpointManager::Ptr mgr = GetIcingaApplication()->GetEndpointManager();
	mgr->OnNewEndpoint += bind_weak(&AuthenticationComponent::NewEndpointHandler, shared_from_this());
	mgr->ForeachEndpoint(bind(&AuthenticationComponent::NewEndpointHandler, this, _1));
	mgr->RegisterEndpoint(m_AuthenticationEndpoint);
}

void AuthenticationComponent::Stop(void)
{
	IcingaApplication::Ptr app = GetIcingaApplication();

	if (app) {
		EndpointManager::Ptr mgr = app->GetEndpointManager();
		mgr->UnregisterEndpoint(m_AuthenticationEndpoint);
	}
}

int AuthenticationComponent::NewEndpointHandler(const NewEndpointEventArgs& neea)
{
	if (neea.Endpoint->IsLocal() || neea.Endpoint->HasIdentity())
		return 0;

	neea.Endpoint->AddAllowedMethodSinkPrefix("auth::");
	neea.Endpoint->AddAllowedMethodSourcePrefix("auth::");

	JsonRpcRequest request;
	request.SetMethod("message::SetIdentity");

	IdentityMessage params;
	params.SetIdentity("keks");
	request.SetParams(params);

	neea.Endpoint->ProcessRequest(m_AuthenticationEndpoint, request);

	return 0;
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
	request.SetMethod("auth::Welcome");
	nrea.Sender->ProcessRequest(m_AuthenticationEndpoint, request);

	return 0;
}
