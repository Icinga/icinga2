#include "i2-icinga.h"

using namespace icinga;

string DiscoveryComponent::GetName(void) const
{
	return "discoverycomponent";
}

void DiscoveryComponent::Start(void)
{
	m_DiscoveryEndpoint = make_shared<VirtualEndpoint>();
	m_DiscoveryEndpoint->RegisterMethodHandler("message::Welcome",
		bind_weak(&DiscoveryComponent::WelcomeMessageHandler, shared_from_this()));

	m_DiscoveryEndpoint->RegisterMethodSource("discovery::PeerAvailable");
	m_DiscoveryEndpoint->RegisterMethodHandler("discovery::GetPeers",
		bind_weak(&DiscoveryComponent::GetPeersMessageHandler, shared_from_this()));

	GetEndpointManager()->RegisterEndpoint(m_DiscoveryEndpoint);
}

void DiscoveryComponent::Stop(void)
{
	EndpointManager::Ptr mgr = GetEndpointManager();

	if (mgr)
		mgr->UnregisterEndpoint(m_DiscoveryEndpoint);
}

int DiscoveryComponent::CheckExistingEndpoint(Endpoint::Ptr endpoint, const NewEndpointEventArgs& neea)
{
	if (endpoint == neea.Endpoint)
		return 0;

	if (endpoint->GetIdentity() == neea.Endpoint->GetIdentity()) {
		Application::Log("Detected duplicate identity (" + endpoint->GetIdentity() + " - Disconnecting endpoint.");

		endpoint->Stop();
		GetEndpointManager()->UnregisterEndpoint(endpoint);
	}

	return 0;
}

int DiscoveryComponent::WelcomeMessageHandler(const NewRequestEventArgs& neea)
{
	if (neea.Sender->GetIdentity() == GetEndpointManager()->GetIdentity()) {
		Application::Log("Detected loop-back connection - Disconnecting endpoint.");

		neea.Sender->Stop();
		GetEndpointManager()->UnregisterEndpoint(neea.Sender);

		return 0;
	}

	GetEndpointManager()->ForeachEndpoint(bind(&DiscoveryComponent::CheckExistingEndpoint, this, neea.Sender, _1));

	JsonRpcRequest request;
	request.SetMethod("discovery::GetPeers");
	GetEndpointManager()->SendUnicastRequest(m_DiscoveryEndpoint, neea.Sender, request);

	/* TODO: send information about this client to all other clients */

	return 0;
}

int DiscoveryComponent::GetPeersMessageHandler(const NewRequestEventArgs& nrea)
{
	/* TODO: send information about all available clients to this client */

	return 0;
}
