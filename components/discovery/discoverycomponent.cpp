/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-discovery.h"

using namespace icinga;

/**
 * Returns the name of this component.
 *
 * @returns The name.
 */
string DiscoveryComponent::GetName(void) const
{
	return "discoverycomponent";
}

/**
 * Starts the discovery component.
 */
void DiscoveryComponent::Start(void)
{
	m_DiscoveryEndpoint = make_shared<VirtualEndpoint>();

	m_DiscoveryEndpoint->RegisterPublication("discovery::RegisterComponent");
	m_DiscoveryEndpoint->RegisterTopicHandler("discovery::RegisterComponent",
		bind_weak(&DiscoveryComponent::RegisterComponentMessageHandler, shared_from_this()));

	m_DiscoveryEndpoint->RegisterPublication("discovery::NewComponent");
	m_DiscoveryEndpoint->RegisterTopicHandler("discovery::NewComponent",
		bind_weak(&DiscoveryComponent::NewComponentMessageHandler, shared_from_this()));

	m_DiscoveryEndpoint->RegisterTopicHandler("discovery::Welcome",
		bind_weak(&DiscoveryComponent::WelcomeMessageHandler, shared_from_this()));

	GetEndpointManager()->ForEachEndpoint(bind(&DiscoveryComponent::NewEndpointHandler, this, _1));
	GetEndpointManager()->OnNewEndpoint += bind_weak(&DiscoveryComponent::NewEndpointHandler, shared_from_this());

	GetEndpointManager()->RegisterEndpoint(m_DiscoveryEndpoint);

	/* create the reconnect timer */
	m_DiscoveryTimer = make_shared<Timer>();
	m_DiscoveryTimer->SetInterval(30);
	m_DiscoveryTimer->OnTimerExpired += bind_weak(&DiscoveryComponent::DiscoveryTimerHandler, shared_from_this());
	m_DiscoveryTimer->Start();

	/* call the timer as soon as possible */
	m_DiscoveryTimer->Reschedule(0);
}

/**
 * Stops the discovery component.
 */
void DiscoveryComponent::Stop(void)
{
	EndpointManager::Ptr mgr = GetEndpointManager();

	if (mgr)
		mgr->UnregisterEndpoint(m_DiscoveryEndpoint);
}

/**
 * Checks whether the specified endpoint is already connected
 * and disconnects older endpoints.
 *
 * @param endpoint The endpoint that is to be checked.
 * @param neea Event arguments for another endpoint.
 * @returns 0
 */
int DiscoveryComponent::CheckExistingEndpoint(Endpoint::Ptr endpoint, const NewEndpointEventArgs& neea)
{
	if (endpoint == neea.Endpoint)
		return 0;

	if (!neea.Endpoint->IsConnected())
		return 0;

	if (endpoint->GetIdentity() == neea.Endpoint->GetIdentity()) {
		Application::Log("Detected duplicate identity:" + endpoint->GetIdentity() + " - Disconnecting old endpoint.");

		neea.Endpoint->Stop();
		GetEndpointManager()->UnregisterEndpoint(neea.Endpoint);
	}

	return 0;
}

/**
 * Registers handlers for new endpoints.
 *
 * @param neea Event arguments for the new endpoint.
 * @returns 0
 */
int DiscoveryComponent::NewEndpointHandler(const NewEndpointEventArgs& neea)
{
	neea.Endpoint->OnIdentityChanged += bind_weak(&DiscoveryComponent::NewIdentityHandler, shared_from_this());

	/* accept discovery::RegisterComponent messages from any endpoint */
	neea.Endpoint->RegisterPublication("discovery::RegisterComponent");

	/* accept discovery::Welcome messages from any endpoint */
	neea.Endpoint->RegisterPublication("discovery::Welcome");

	return 0;
}

/**
 * Registers message Subscriptions/sources in the specified component information object.
 *
 * @param neea Event arguments for the endpoint.
 * @param info Component information object.
 * @return 0
 */
int DiscoveryComponent::DiscoveryEndpointHandler(const NewEndpointEventArgs& neea, ComponentDiscoveryInfo::Ptr info) const
{
	Endpoint::ConstTopicIterator i;

	for (i = neea.Endpoint->BeginSubscriptions(); i != neea.Endpoint->EndSubscriptions(); i++) {
		info->Subscriptions.insert(*i);
	}

	for (i = neea.Endpoint->BeginPublications(); i != neea.Endpoint->EndPublications(); i++) {
		info->Publications.insert(*i);
	}

	return 0;
}

/**
 * Retrieves the component information object for the specified component.
 *
 * @param component The identity of the component.
 * @param info Pointer to the information object.
 * @returns true if the info object was successfully retrieved, false otherwise.
 */
bool DiscoveryComponent::GetComponentDiscoveryInfo(string component, ComponentDiscoveryInfo::Ptr *info) const
{
	if (component == GetEndpointManager()->GetIdentity()) {
		/* Build fake discovery info for ourselves */
		*info = make_shared<ComponentDiscoveryInfo>();
		GetEndpointManager()->ForEachEndpoint(bind(&DiscoveryComponent::DiscoveryEndpointHandler, this, _1, *info));
		
		(*info)->LastSeen = 0;
		(*info)->Node = GetIcingaApplication()->GetNode();
		(*info)->Service = GetIcingaApplication()->GetService();

		return true;
	}

	map<string, ComponentDiscoveryInfo::Ptr>::const_iterator i;

	i = m_Components.find(component);

	if (i == m_Components.end())
		return false;

	*info = i->second;
	return true;
}

/**
 * Deals with a new endpoint whose identity has just become known.
 *
 * @param ea Event arguments for the component.
 * @returns 0
 */
int DiscoveryComponent::NewIdentityHandler(const EventArgs& ea)
{
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(ea.Source);
	string identity = endpoint->GetIdentity();

	if (identity == GetEndpointManager()->GetIdentity()) {
		Application::Log("Detected loop-back connection - Disconnecting endpoint.");

		endpoint->Stop();
		GetEndpointManager()->UnregisterEndpoint(endpoint);

		return 0;
	}

	GetEndpointManager()->ForEachEndpoint(bind(&DiscoveryComponent::CheckExistingEndpoint, this, endpoint, _1));

	// we assume the other component _always_ wants
	// discovery::RegisterComponent messages from us
	endpoint->RegisterSubscription("discovery::RegisterComponent");

	// send a discovery::RegisterComponent message, if the
	// other component is a broker this makes sure
	// the broker knows about our message types
	SendDiscoveryMessage("discovery::RegisterComponent", GetEndpointManager()->GetIdentity(), endpoint);

	map<string, ComponentDiscoveryInfo::Ptr>::iterator ic;

	// we assume the other component _always_ wants
	// discovery::NewComponent messages from us
	endpoint->RegisterSubscription("discovery::NewComponent");

	// send discovery::NewComponent message for ourselves
	SendDiscoveryMessage("discovery::NewComponent", GetEndpointManager()->GetIdentity(), endpoint);

	// send discovery::NewComponent messages for all components
	// we know about
	for (ic = m_Components.begin(); ic != m_Components.end(); ic++) {
		SendDiscoveryMessage("discovery::NewComponent", ic->first, endpoint);
	}

	// check if we already know the other component
	ic = m_Components.find(endpoint->GetIdentity());

	if (ic == m_Components.end()) {
		// we don't know the other component yet, so
		// wait until we get a discovery::NewComponent message
		// from a broker
		return 0;
	}

	// register published/subscribed topics for this endpoint
	ComponentDiscoveryInfo::Ptr info = ic->second;
	set<string>::iterator it;
	for (it = info->Publications.begin(); it != info->Publications.end(); it++)
		endpoint->RegisterPublication(*it);

	for (it = info->Subscriptions.begin(); it != info->Subscriptions.end(); it++)
		endpoint->RegisterSubscription(*it);

	FinishDiscoverySetup(endpoint);

	return 0;
}

/**
 * Processes discovery::Welcome messages.
 *
 * @param nrea Event arguments for the request.
 * @returns 0
 */
int DiscoveryComponent::WelcomeMessageHandler(const NewRequestEventArgs& nrea)
{
	Endpoint::Ptr endpoint = nrea.Sender;

	if (endpoint->GetReceivedWelcome())
		return 0;

	endpoint->SetReceivedWelcome(true);

	if (endpoint->GetSentWelcome()) {
		EventArgs ea;
		ea.Source = endpoint;
		endpoint->OnSessionEstablished(ea);
	}

	return 0;
}

/**
 * Finishes the welcome handshake for a new component
 * by registering message Subscriptions/sources for the component
 * and sending a welcome message if necessary.
 *
 * @param endpoint The endpoint to set up.
 */
void DiscoveryComponent::FinishDiscoverySetup(Endpoint::Ptr endpoint)
{
	if (endpoint->GetSentWelcome())
		return;

	// we assume the other component _always_ wants
	// discovery::Welcome messages from us
	endpoint->RegisterSubscription("discovery::Welcome");
	RpcRequest request;
	request.SetMethod("discovery::Welcome");
	GetEndpointManager()->SendUnicastMessage(m_DiscoveryEndpoint, endpoint, request);

	endpoint->SetSentWelcome(true);

	if (endpoint->GetReceivedWelcome()) {
		EventArgs ea;
		ea.Source = endpoint;
		endpoint->OnSessionEstablished(ea);
	}
}

/**
 * Sends a discovery message for the specified identity using the
 * specified message type.
 *
 * @param method The method to use for the message ("discovery::NewComponent" or "discovery::RegisterComponent").
 * @param identity The identity of the component for which a message should be sent.
 * @param recipient The recipient of the message. A multicast message is sent if this parameter is empty.
 */
void DiscoveryComponent::SendDiscoveryMessage(string method, string identity, Endpoint::Ptr recipient)
{
	RpcRequest request;
	request.SetMethod(method);
	
	DiscoveryMessage params;
	request.SetParams(params);

	params.SetIdentity(identity);

	Message subscriptions;
	params.SetSubscriptions(subscriptions);

	Message publications;
	params.SetPublications(publications);

	ComponentDiscoveryInfo::Ptr info;

	if (!GetComponentDiscoveryInfo(identity, &info))
		return;

	if (!info->Node.empty() && !info->Service.empty()) {
		params.SetNode(info->Node);
		params.SetService(info->Service);
	}

	set<string>::iterator i;
	for (i = info->Publications.begin(); i != info->Publications.end(); i++)
		publications.AddUnnamedPropertyString(*i);

	for (i = info->Subscriptions.begin(); i != info->Subscriptions.end(); i++)
		subscriptions.AddUnnamedPropertyString(*i);

	if (recipient)
		GetEndpointManager()->SendUnicastMessage(m_DiscoveryEndpoint, recipient, request);
	else
		GetEndpointManager()->SendMulticastMessage(m_DiscoveryEndpoint, request);
}

bool DiscoveryComponent::HasMessagePermission(Dictionary::Ptr roles, string messageType, string message)
{
	if (!roles)
		return false;

	ConfigHive::Ptr configHive = GetApplication()->GetConfigHive();
	ConfigCollection::Ptr roleCollection = configHive->GetCollection("role");

	for (DictionaryIterator ip = roles->Begin(); ip != roles->End(); ip++) {
		ConfigObject::Ptr role = roleCollection->GetObject(ip->second);

		if (!role)
			continue;

		Dictionary::Ptr permissions;
		if (!role->GetPropertyDictionary(messageType, &permissions))
			continue;

		for (DictionaryIterator is = permissions->Begin(); is != permissions->End(); is++) {
			if (Utility::Match(is->second.GetString(), message))
				return true;
		}
	}

	return false;
}

/**
 * Processes a discovery message by registering the component in the
 * discovery component registry.
 *
 * @param identity The authorative identity of the component.
 * @param message The discovery message.
 * @param trusted Whether the message comes from a trusted source (i.e. a broker).
 */
void DiscoveryComponent::ProcessDiscoveryMessage(string identity, DiscoveryMessage message, bool trusted)
{
	/* ignore discovery messages that are about ourselves */
	if (identity == GetEndpointManager()->GetIdentity())
		return;

	ComponentDiscoveryInfo::Ptr info = make_shared<ComponentDiscoveryInfo>();

	time(&(info->LastSeen));

	message.GetNode(&info->Node);
	message.GetService(&info->Service);

	ConfigHive::Ptr configHive = GetApplication()->GetConfigHive();
	ConfigCollection::Ptr endpointCollection = configHive->GetCollection("endpoint");

	ConfigObject::Ptr endpointConfig = endpointCollection->GetObject(identity);
	Dictionary::Ptr roles;
	if (endpointConfig)
		endpointConfig->GetPropertyDictionary("roles", &roles);

	Endpoint::Ptr endpoint = GetEndpointManager()->GetEndpointByIdentity(identity);

	Message publications;
	if (message.GetPublications(&publications)) {
		DictionaryIterator i;
		for (i = publications.GetDictionary()->Begin(); i != publications.GetDictionary()->End(); i++) {
			if (trusted || HasMessagePermission(roles, "publications", i->second)) {
				info->Publications.insert(i->second);
				if (endpoint)
					endpoint->RegisterPublication(i->second);
			}
		}
	}

	Message subscriptions;
	if (message.GetSubscriptions(&subscriptions)) {
		DictionaryIterator i;
		for (i = subscriptions.GetDictionary()->Begin(); i != subscriptions.GetDictionary()->End(); i++) {
			if (trusted || HasMessagePermission(roles, "subscriptions", i->second)) {
				info->Subscriptions.insert(i->second);
				if (endpoint)
					endpoint->RegisterSubscription(i->second);
			}
		}
	}

	map<string, ComponentDiscoveryInfo::Ptr>::iterator i;

	i = m_Components.find(identity);

	if (i != m_Components.end())
		m_Components.erase(i);

	m_Components[identity] = info;

	SendDiscoveryMessage("discovery::NewComponent", identity, Endpoint::Ptr());

	/* don't send a welcome message for discovery::RegisterComponent messages */
	if (endpoint && trusted)
		FinishDiscoverySetup(endpoint);
}

/**
 * Processes "discovery::NewComponent" messages.
 *
 * @param nrea Event arguments for the request.
 * @returns 0
 */
int DiscoveryComponent::NewComponentMessageHandler(const NewRequestEventArgs& nrea)
{
	DiscoveryMessage message;
	nrea.Request.GetParams(&message);

	string identity;
	if (!message.GetIdentity(&identity))
		return 0;

	ProcessDiscoveryMessage(identity, message, true);
	return 0;
}

/**
 * Processes "discovery::RegisterComponent" messages.
 *
 * @param nrea Event arguments for the request.
 * @returns 0
 */
int DiscoveryComponent::RegisterComponentMessageHandler(const NewRequestEventArgs& nrea)
{
	DiscoveryMessage message;
	nrea.Request.GetParams(&message);
	ProcessDiscoveryMessage(nrea.Sender->GetIdentity(), message, false);

	return 0;
}

/**
 * Processes "endpoint" config objects.
 *
 * @param ea Event arguments for the new config object.
 * @returns 0
 */
int DiscoveryComponent::EndpointConfigHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);

	EndpointManager::Ptr endpointManager = GetEndpointManager();

	/* Check if we're already connected to this endpoint. */
	if (endpointManager->GetEndpointByIdentity(object->GetName()))
		return 0;

	string node, service;
	if (object->GetPropertyString("node", &node) && object->GetPropertyString("service", &service)) {
		/* reconnect to this endpoint */
		endpointManager->AddConnection(node, service);
	}

	return 0;
}

/**
 * Checks whether we have to reconnect to other components and removes stale
 * components from the registry.
 *
 * @param tea Event arguments for the timer.
 * @returns 0
 */
int DiscoveryComponent::DiscoveryTimerHandler(const TimerEventArgs& tea)
{
	EndpointManager::Ptr endpointManager = GetEndpointManager();
	
	time_t now;
	time(&now);

	/* check whether we have to reconnect to one of our upstream endpoints */
	ConfigCollection::Ptr endpointCollection = GetApplication()->GetConfigHive()->GetCollection("endpoint");
	endpointCollection->ForEachObject(bind(&DiscoveryComponent::EndpointConfigHandler, this, _1));

	map<string, ComponentDiscoveryInfo::Ptr>::iterator curr, i;
	for (i = m_Components.begin(); i != m_Components.end(); ) {
		string identity = i->first;
		ComponentDiscoveryInfo::Ptr info = i->second;

		curr = i;
		i++;

		if (info->LastSeen < now - DiscoveryComponent::RegistrationTTL) {
			/* unregister this component if its registration has expired */
			m_Components.erase(curr);
			continue;
		}

		/* send discovery message to all connected components to
			refresh their TTL for this component */
		SendDiscoveryMessage("discovery::NewComponent", identity, Endpoint::Ptr());

		Endpoint::Ptr endpoint = endpointManager->GetEndpointByIdentity(identity);
		if (endpoint && endpoint->IsConnected()) {
			/* update LastSeen if we're still connected to this endpoint */
			info->LastSeen = now;
		} else {
			/* TODO: figure out whether we actually want to connect to this component */
			/* try and reconnect to this component */
			endpointManager->AddConnection(info->Node, info->Service);
		}
	}

	return 0;
}

EXPORT_COMPONENT(discovery, DiscoveryComponent);
