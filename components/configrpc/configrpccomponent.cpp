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
	ConfigHive::Ptr configHive = GetConfigHive();

	m_ConfigRpcEndpoint = make_shared<VirtualEndpoint>();

	long configSource;
	if (GetConfig()->GetProperty("configSource", &configSource) && configSource != 0) {
		m_ConfigRpcEndpoint->RegisterTopicHandler("config::FetchObjects",
		    bind_weak(&ConfigRpcComponent::FetchObjectsHandler, shared_from_this()));

		configHive->OnObjectCommitted += bind_weak(&ConfigRpcComponent::LocalObjectCommittedHandler, shared_from_this());
		configHive->OnObjectRemoved += bind_weak(&ConfigRpcComponent::LocalObjectRemovedHandler, shared_from_this());

		m_ConfigRpcEndpoint->RegisterPublication("config::ObjectCommitted");
		m_ConfigRpcEndpoint->RegisterPublication("config::ObjectRemoved");
	}

	endpointManager->OnNewEndpoint += bind_weak(&ConfigRpcComponent::NewEndpointHandler, shared_from_this());

	m_ConfigRpcEndpoint->RegisterPublication("config::FetchObjects");
	m_ConfigRpcEndpoint->RegisterTopicHandler("config::ObjectCommitted",
	    bind_weak(&ConfigRpcComponent::RemoteObjectCommittedHandler, shared_from_this()));
	m_ConfigRpcEndpoint->RegisterTopicHandler("config::ObjectRemoved",
	    bind_weak(&ConfigRpcComponent::RemoteObjectRemovedHandler, shared_from_this()));

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
	ea.Endpoint->OnSessionEstablished += bind_weak(&ConfigRpcComponent::SessionEstablishedHandler, shared_from_this());

	return 0;
}

int ConfigRpcComponent::SessionEstablishedHandler(const EventArgs& ea)
{
	RpcRequest request;
	request.SetMethod("config::FetchObjects");

	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(ea.Source);
	GetEndpointManager()->SendUnicastMessage(m_ConfigRpcEndpoint, endpoint, request);

	return 0;
}

RpcRequest ConfigRpcComponent::MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties)
{
	RpcRequest msg;
	msg.SetMethod(method);

	MessagePart params;
	msg.SetParams(params);

	params.SetProperty("name", object->GetName());
	params.SetProperty("type", object->GetType());

	if (includeProperties)
		params.SetProperty("properties", object);

	return msg;
}

bool ConfigRpcComponent::ShouldReplicateObject(const ConfigObject::Ptr& object)
{
	long replicate;
	if (!object->GetProperty("replicate", &replicate))
		return true;
	return (replicate != 0);
}

int ConfigRpcComponent::FetchObjectsHandler(const NewRequestEventArgs& ea)
{
	Endpoint::Ptr client = ea.Sender;
	ConfigHive::Ptr configHive = GetConfigHive();

	for (ConfigHive::CollectionIterator ci = configHive->Collections.begin(); ci != configHive->Collections.end(); ci++) {
		ConfigCollection::Ptr collection = ci->second;

		for (ConfigCollection::ObjectIterator oi = collection->Objects.begin(); oi != collection->Objects.end(); oi++) {
			ConfigObject::Ptr object = oi->second;

			if (!ShouldReplicateObject(object))
				continue;

			RpcRequest request = MakeObjectMessage(object, "config::ObjectCreated", true);

			GetEndpointManager()->SendUnicastMessage(m_ConfigRpcEndpoint, client, request);
		}
	}

	return 0;
}

int ConfigRpcComponent::LocalObjectCommittedHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	if (!ShouldReplicateObject(object))
		return 0;

	GetEndpointManager()->SendMulticastMessage(m_ConfigRpcEndpoint,
	    MakeObjectMessage(object, "config::ObjectCreated", true));

	return 0;
}

int ConfigRpcComponent::LocalObjectRemovedHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	if (!ShouldReplicateObject(object))
		return 0;

	GetEndpointManager()->SendMulticastMessage(m_ConfigRpcEndpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", false));

	return 0;
}

int ConfigRpcComponent::RemoteObjectCommittedHandler(const NewRequestEventArgs& ea)
{
	RpcRequest message = ea.Request;
	bool was_null = false;

	MessagePart params;
	if (!message.GetParams(&params))
		return 0;

	string name;
	if (!params.GetProperty("name", &name))
		return 0;

	string type;
	if (!params.GetProperty("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object) {
		was_null = true;
		object = make_shared<ConfigObject>(type, name);
	}

	MessagePart properties;
	if (!params.GetProperty("properties", &properties))
		return 0;

	for (DictionaryIterator i = properties.Begin(); i != properties.End(); i++) {
		object->SetProperty(i->first, i->second);
	}

	if (was_null) {
		object->SetReplicated(true);
		configHive->AddObject(object);
	}

	return 0;
}

int ConfigRpcComponent::RemoteObjectRemovedHandler(const NewRequestEventArgs& ea)
{
	RpcRequest message = ea.Request;
	
	MessagePart params;
	if (!message.GetParams(&params))
		return 0;

	string name;
	if (!params.GetProperty("name", &name))
		return 0;

	string type;
	if (!params.GetProperty("type", &type))
		return 0;

	ConfigHive::Ptr configHive = GetConfigHive();
	ConfigObject::Ptr object = configHive->GetObject(type, name);

	if (!object)
		return 0;

	if (object->GetReplicated())
		configHive->RemoveObject(object);

	return 0;
}

EXPORT_COMPONENT(configrpc, ConfigRpcComponent);
