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
	m_Endpoint = boost::make_shared<VirtualEndpoint>();

	m_Endpoint->RegisterPublication("discovery::RegisterComponent");
	m_Endpoint->RegisterTopicHandler("discovery::RegisterComponent",
		boost::bind(&DiscoveryComponent::RegisterComponentMessageHandler, this, _2, _3));

	m_Endpoint->RegisterPublication("discovery::NewComponent");
	m_Endpoint->RegisterTopicHandler("discovery::NewComponent",
		boost::bind(&DiscoveryComponent::NewComponentMessageHandler, this, _3));

	m_Endpoint->RegisterTopicHandler("discovery::Welcome",
		boost::bind(&DiscoveryComponent::WelcomeMessageHandler, this, _2, _3));

	EndpointManager::GetInstance()->ForEachEndpoint(boost::bind(&DiscoveryComponent::NewEndpointHandler, this, _2));
	EndpointManager::GetInstance()->OnNewEndpoint.connect(boost::bind(&DiscoveryComponent::NewEndpointHandler, this, _2));

	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);

	/* create the reconnect timer */
	m_DiscoveryTimer = boost::make_shared<Timer>();
	m_DiscoveryTimer->SetInterval(30);
	m_DiscoveryTimer->OnTimerExpired.connect(boost::bind(&DiscoveryComponent::DiscoveryTimerHandler, this));
	m_DiscoveryTimer->Start();

	/* call the timer as soon as possible */
	m_DiscoveryTimer->Reschedule(0);

	CIB::RequireInformation(CIB_Configuration);
}

/**
 * Stops the discovery component.
 */
void DiscoveryComponent::Stop(void)
{
	EndpointManager::Ptr mgr = EndpointManager::GetInstance();

	if (mgr)
		mgr->UnregisterEndpoint(m_Endpoint);
}

/**
 * Checks whether the specified endpoint is already connected
 * and disconnects older endpoints.
 *
 * @param self The endpoint that is to be checked.
 * @param other The other endpoint.
 */
void DiscoveryComponent::CheckExistingEndpoint(const Endpoint::Ptr& self, const Endpoint::Ptr& other)
{
	if (self == other)
		return;

	if (!other->IsConnected())
		return;

	if (self->GetIdentity() == other->GetIdentity()) {
		Logger::Write(LogWarning, "discovery", "Detected duplicate identity:" + other->GetIdentity() + " - Disconnecting old endpoint.");

		other->Stop();
		EndpointManager::GetInstance()->UnregisterEndpoint(other);
	}
}

/**
 * Deals with a new endpoint.
 *
 * @param endpoint The endpoint.
 */
void DiscoveryComponent::NewEndpointHandler(const Endpoint::Ptr& endpoint)
{
	/* immediately finish session setup for local endpoints */
	if (endpoint->IsLocal()) {
		endpoint->OnSessionEstablished(endpoint);
		return;
	}

	/* accept discovery::RegisterComponent messages from any endpoint */
	endpoint->RegisterPublication("discovery::RegisterComponent");

	/* accept discovery::Welcome messages from any endpoint */
	endpoint->RegisterPublication("discovery::Welcome");

	string identity = endpoint->GetIdentity();

	if (identity == EndpointManager::GetInstance()->GetIdentity()) {
		Logger::Write(LogWarning, "discovery", "Detected loop-back connection - Disconnecting endpoint.");

		endpoint->Stop();
		EndpointManager::GetInstance()->UnregisterEndpoint(endpoint);

		return;
	}

	EndpointManager::GetInstance()->ForEachEndpoint(boost::bind(&DiscoveryComponent::CheckExistingEndpoint, this, endpoint, _2));

	// we assume the other component _always_ wants
	// discovery::RegisterComponent messages from us
	endpoint->RegisterSubscription("discovery::RegisterComponent");

	// send a discovery::RegisterComponent message, if the
	// other component is a broker this makes sure
	// the broker knows about our message types
	SendDiscoveryMessage("discovery::RegisterComponent", EndpointManager::GetInstance()->GetIdentity(), endpoint);

	map<string, ComponentDiscoveryInfo::Ptr>::iterator ic;

	// we assume the other component _always_ wants
	// discovery::NewComponent messages from us
	endpoint->RegisterSubscription("discovery::NewComponent");

	// send discovery::NewComponent message for ourselves
	SendDiscoveryMessage("discovery::NewComponent", EndpointManager::GetInstance()->GetIdentity(), endpoint);

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
		return;
	}

	// register published/subscribed topics for this endpoint
	ComponentDiscoveryInfo::Ptr info = ic->second;
	BOOST_FOREACH(const string& publication, info->Publications) {
		endpoint->RegisterPublication(publication);
	}

	BOOST_FOREACH(const string& subscription, info->Subscriptions) {
		endpoint->RegisterSubscription(subscription);
	}

	FinishDiscoverySetup(endpoint);
}

/**
 * Registers message Subscriptions/sources in the specified component information object.
 *
 * @param neea Event arguments for the endpoint.
 * @param info Component information object.
 * @return 0
 */
void DiscoveryComponent::DiscoveryEndpointHandler(const Endpoint::Ptr& endpoint, const ComponentDiscoveryInfo::Ptr& info) const
{
	Endpoint::ConstTopicIterator i;

	for (i = endpoint->BeginSubscriptions(); i != endpoint->EndSubscriptions(); i++)
		info->Subscriptions.insert(*i);

	for (i = endpoint->BeginPublications(); i != endpoint->EndPublications(); i++)
		info->Publications.insert(*i);
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
	if (component == EndpointManager::GetInstance()->GetIdentity()) {
		/* Build fake discovery info for ourselves */
		*info = boost::make_shared<ComponentDiscoveryInfo>();
		EndpointManager::GetInstance()->ForEachEndpoint(boost::bind(&DiscoveryComponent::DiscoveryEndpointHandler, this, _2, *info));
		
		(*info)->LastSeen = 0;
		(*info)->Node = IcingaApplication::GetInstance()->GetNode();
		(*info)->Service = IcingaApplication::GetInstance()->GetService();

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
 * Processes discovery::Welcome messages.
 *
 * @param nrea Event arguments for the request.
 * @returns 0
 */
void DiscoveryComponent::WelcomeMessageHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	if (sender->HasReceivedWelcome())
		return;

	sender->SetReceivedWelcome(true);

	if (sender->HasSentWelcome())
		sender->OnSessionEstablished(sender);
}

/**
 * Finishes the welcome handshake for a new component
 * by registering message Subscriptions/sources for the component
 * and sending a welcome message if necessary.
 *
 * @param endpoint The endpoint to set up.
 */
void DiscoveryComponent::FinishDiscoverySetup(const Endpoint::Ptr& endpoint)
{
	if (endpoint->HasSentWelcome())
		return;

	// we assume the other component _always_ wants
	// discovery::Welcome messages from us
	endpoint->RegisterSubscription("discovery::Welcome");
	RequestMessage request;
	request.SetMethod("discovery::Welcome");
	EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, endpoint, request);

	endpoint->SetSentWelcome(true);

	if (endpoint->HasReceivedWelcome())
		endpoint->OnSessionEstablished(endpoint);
}

/**
 * Sends a discovery message for the specified identity using the
 * specified message type.
 *
 * @param method The method to use for the message ("discovery::NewComponent" or "discovery::RegisterComponent").
 * @param identity The identity of the component for which a message should be sent.
 * @param recipient The recipient of the message. A multicast message is sent if this parameter is empty.
 */
void DiscoveryComponent::SendDiscoveryMessage(const string& method, const string& identity, const Endpoint::Ptr& recipient)
{
	RequestMessage request;
	request.SetMethod(method);
	
	DiscoveryMessage params;
	request.SetParams(params);

	params.SetIdentity(identity);

	ComponentDiscoveryInfo::Ptr info;

	if (!GetComponentDiscoveryInfo(identity, &info))
		return;

	if (!info->Node.empty() && !info->Service.empty()) {
		params.SetNode(info->Node);
		params.SetService(info->Service);
	}

	set<string>::iterator i;
	Dictionary::Ptr subscriptions = boost::make_shared<Dictionary>();
	BOOST_FOREACH(const string &subscription, info->Subscriptions) {
		subscriptions->Add(subscription);
	}

	params.SetSubscriptions(subscriptions);

	Dictionary::Ptr publications = boost::make_shared<Dictionary>();
	BOOST_FOREACH(const string& publication, info->Publications) {
		publications->Add(publication);
	}

	params.SetPublications(publications);

	if (recipient)
		EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, recipient, request);
	else
		EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint, request);
}

bool DiscoveryComponent::HasMessagePermission(const Dictionary::Ptr& roles, const string& messageType, const string& message)
{
	if (!roles)
		return false;

	ConfigObject::TMap::Range range = ConfigObject::GetObjects("role");

	BOOST_FOREACH(const ConfigObject::Ptr& role, range | map_values) {
		Dictionary::Ptr permissions;
		if (!role->GetProperty(messageType, &permissions))
			continue;

		BOOST_FOREACH(const Variant& permission, permissions | map_values) {
			if (Utility::Match(permission, message))
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
void DiscoveryComponent::ProcessDiscoveryMessage(const string& identity, const DiscoveryMessage& message, bool trusted)
{
	/* ignore discovery messages that are about ourselves */
	if (identity == EndpointManager::GetInstance()->GetIdentity())
		return;

	ComponentDiscoveryInfo::Ptr info = boost::make_shared<ComponentDiscoveryInfo>();

	time(&(info->LastSeen));

	string node;
	if (message.GetNode(&node) && !node.empty())
		info->Node = node;

	string service;
	if (message.GetService(&service) && !service.empty())
		info->Service = service;

	ConfigObject::Ptr endpointConfig = ConfigObject::GetObject("endpoint", identity);
	Dictionary::Ptr roles;
	if (endpointConfig)
		endpointConfig->GetProperty("roles", &roles);

	Endpoint::Ptr endpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(identity);

	Dictionary::Ptr publications;
	if (message.GetPublications(&publications)) {
		BOOST_FOREACH(const Variant& publication, publications | map_values) {
			if (trusted || HasMessagePermission(roles, "publications", publication)) {
				info->Publications.insert(publication);
				if (endpoint)
					endpoint->RegisterPublication(publication);
			}
		}
	}

	Dictionary::Ptr subscriptions;
	if (message.GetSubscriptions(&subscriptions)) {
		BOOST_FOREACH(const Variant& subscription, subscriptions | map_values) {
			if (trusted || HasMessagePermission(roles, "subscriptions", subscription)) {
				info->Subscriptions.insert(subscription);
				if (endpoint)
					endpoint->RegisterSubscription(subscription);
			}
		}
	}

	map<string, ComponentDiscoveryInfo::Ptr>::iterator i;

	i = m_Components.find(identity);

	if (i != m_Components.end())
		m_Components.erase(i);

	m_Components[identity] = info;

	SendDiscoveryMessage("discovery::NewComponent", identity, Endpoint::Ptr());

	/* don't send a welcome message for discovery::NewComponent messages */
	if (endpoint && !trusted)
		FinishDiscoverySetup(endpoint);
}

/**
 * Processes "discovery::NewComponent" messages.
 *
 * @param nrea Event arguments for the request.
 */
void DiscoveryComponent::NewComponentMessageHandler(const RequestMessage& request)
{
	DiscoveryMessage message;
	request.GetParams(&message);

	string identity;
	if (!message.GetIdentity(&identity))
		return;

	ProcessDiscoveryMessage(identity, message, true);
}

/**
 * Processes "discovery::RegisterComponent" messages.
 *
 * @param nrea Event arguments for the request.
 */
void DiscoveryComponent::RegisterComponentMessageHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	DiscoveryMessage message;
	request.GetParams(&message);
	ProcessDiscoveryMessage(sender->GetIdentity(), message, false);
}

/**
 * Checks whether we have to reconnect to other components and removes stale
 * components from the registry.
 */
void DiscoveryComponent::DiscoveryTimerHandler(void)
{
	EndpointManager::Ptr endpointManager = EndpointManager::GetInstance();
	
	time_t now;
	time(&now);

	/* check whether we have to reconnect to one of our upstream endpoints */
	ConfigObject::TMap::Range range = ConfigObject::GetObjects("endpoint");

	BOOST_FOREACH(const ConfigObject::Ptr& object, range | map_values) {
		/* Check if we're already connected to this endpoint. */
		if (endpointManager->GetEndpointByIdentity(object->GetName()))
			continue;

		string node, service;
		if (object->GetProperty("node", &node) && object->GetProperty("service", &service)) {
			/* reconnect to this endpoint */
			endpointManager->AddConnection(node, service);
		}
	}

	map<string, ComponentDiscoveryInfo::Ptr>::iterator curr, i;
	for (i = m_Components.begin(); i != m_Components.end(); ) {
		const string& identity = i->first;
		const ComponentDiscoveryInfo::Ptr& info = i->second;

		curr = i;
		i++;

		/* there's no need to reconnect to ourself */
		if (identity == EndpointManager::GetInstance()->GetIdentity())
			continue;

		/* for explicitly-configured upstream endpoints
		 * we prefer to use the node/service from the
		 * config object - which is what the for loop above does */
		if (ConfigObject::GetObject("endpoint", identity))
			continue;

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
			try {
				if (!info->Node.empty() && !info->Service.empty())
					endpointManager->AddConnection(info->Node, info->Service);
			} catch (const exception& ex) {
				stringstream msgbuf;
				msgbuf << "Exception while trying to reconnect to endpoint '" << endpoint->GetIdentity() << "': " << ex.what();;
				Logger::Write(LogInformation, "discovery", msgbuf.str());
			}
		}
	}
}

EXPORT_COMPONENT(discovery, DiscoveryComponent);
