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
		bind_weak(&DiscoveryComponent::GetPeersMessageHandler, shared_from_this()));

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

int DiscoveryComponent::WelcomeMessageHandler(const NewRequestEventArgs& neea)
{
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
