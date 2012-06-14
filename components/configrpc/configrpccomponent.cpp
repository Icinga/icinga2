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

#include "i2-configrpc.h"

using namespace icinga;

string ConfigRpcComponent::GetName(void) const
{
	return "configcomponent";
}

void ConfigRpcComponent::Start(void)
{
	EndpointManager::Ptr endpointManager = GetEndpointManager();

	m_ConfigRpcEndpoint = make_shared<VirtualEndpoint>();

	long configSource;
	if (GetConfig()->GetProperty("configSource", &configSource) && configSource != 0) {
		m_ConfigRpcEndpoint->RegisterTopicHandler("config::FetchObjects",
		    bind(&ConfigRpcComponent::FetchObjectsHandler, this, _1));

		ConfigObject::GetAllObjects()->OnObjectAdded.connect(bind(&ConfigRpcComponent::LocalObjectCommittedHandler, this, _1));
		ConfigObject::GetAllObjects()->OnObjectCommitted.connect(bind(&ConfigRpcComponent::LocalObjectCommittedHandler, this, _1));
		ConfigObject::GetAllObjects()->OnObjectRemoved.connect(bind(&ConfigRpcComponent::LocalObjectRemovedHandler, this, _1));

		m_ConfigRpcEndpoint->RegisterPublication("config::ObjectCommitted");
		m_ConfigRpcEndpoint->RegisterPublication("config::ObjectRemoved");
	}

	endpointManager->OnNewEndpoint.connect(bind(&ConfigRpcComponent::NewEndpointHandler, this, _1));

	m_ConfigRpcEndpoint->RegisterPublication("config::FetchObjects");
	m_ConfigRpcEndpoint->RegisterTopicHandler("config::ObjectCommitted",
	    bind(&ConfigRpcComponent::RemoteObjectCommittedHandler, this, _1));
	m_ConfigRpcEndpoint->RegisterTopicHandler("config::ObjectRemoved",
	    bind(&ConfigRpcComponent::RemoteObjectRemovedHandler, this, _1));

	endpointManager->RegisterEndpoint(m_ConfigRpcEndpoint);
}

void ConfigRpcComponent::Stop(void)
{
	EndpointManager::Ptr mgr = GetEndpointManager();

	if (mgr)
		mgr->UnregisterEndpoint(m_ConfigRpcEndpoint);
}

int ConfigRpcComponent::NewEndpointHandler(const NewEndpointEventArgs& ea)
{
	ea.Endpoint->OnSessionEstablished.connect(bind(&ConfigRpcComponent::SessionEstablishedHandler, this, _1));

	return 0;
}

int ConfigRpcComponent::SessionEstablishedHandler(const EventArgs& ea)
{
	RequestMessage request;
	request.SetMethod("config::FetchObjects");

	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(ea.Source);
	GetEndpointManager()->SendUnicastMessage(m_ConfigRpcEndpoint, endpoint, request);

	return 0;
}

RequestMessage ConfigRpcComponent::MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties)
{
	RequestMessage msg;
	msg.SetMethod(method);

	MessagePart params;
	msg.SetParams(params);

	params.SetProperty("name", object->GetName());
	params.SetProperty("type", object->GetType());

	if (includeProperties)
		params.SetProperty("properties", object->GetProperties());

	return msg;
}

bool ConfigRpcComponent::ShouldReplicateObject(const ConfigObject::Ptr& object)
{
	return (!object->IsLocal());
}

int ConfigRpcComponent::FetchObjectsHandler(const NewRequestEventArgs& ea)
{
	Endpoint::Ptr client = ea.Sender;
	ConfigObject::Set::Ptr allObjects = ConfigObject::GetAllObjects();

	for (ConfigObject::Set::Iterator ci = allObjects->Begin(); ci != allObjects->End(); ci++) {
		ConfigObject::Ptr object = *ci;

		if (!ShouldReplicateObject(object))
			continue;

		RequestMessage request = MakeObjectMessage(object, "config::ObjectCreated", true);

		GetEndpointManager()->SendUnicastMessage(m_ConfigRpcEndpoint, client, request);
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectCommittedHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	ConfigObject::Ptr object = ea.Target;
	
	if (!ShouldReplicateObject(object))
		return 0;

	GetEndpointManager()->SendMulticastMessage(m_ConfigRpcEndpoint,
	    MakeObjectMessage(object, "config::ObjectCreated", true));

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	ConfigObject::Ptr object = ea.Target;
	
	if (!ShouldReplicateObject(object))
		return 0;

	GetEndpointManager()->SendMulticastMessage(m_ConfigRpcEndpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", false));

	return 0;
}

int ConfigRpcComponent::RemoteObjectCommittedHandler(const NewRequestEventArgs& ea)
{
	RequestMessage message = ea.Request;

	MessagePart params;
	if (!message.GetParams(&params))
		return 0;

	string name;
	if (!params.GetProperty("name", &name))
		return 0;

	string type;
	if (!params.GetProperty("type", &type))
		return 0;

	MessagePart properties;
	if (!params.GetProperty("properties", &properties))
		return 0;

	ConfigObject::Ptr object = ConfigObject::GetObject(type, name);

	if (!object)
		object = make_shared<ConfigObject>(properties.GetDictionary());
	else
		object->SetProperties(properties.GetDictionary());

	object->Commit();

	return 0;
}

int ConfigRpcComponent::RemoteObjectRemovedHandler(const NewRequestEventArgs& ea)
{
	RequestMessage message = ea.Request;
	
	MessagePart params;
	if (!message.GetParams(&params))
		return 0;

	string name;
	if (!params.GetProperty("name", &name))
		return 0;

	string type;
	if (!params.GetProperty("type", &type))
		return 0;

	ConfigObject::Ptr object = ConfigObject::GetObject(type, name);

	if (!object)
		return 0;

	if (!object->IsLocal())
		object->Unregister();

	return 0;
}

EXPORT_COMPONENT(configrpc, ConfigRpcComponent);
