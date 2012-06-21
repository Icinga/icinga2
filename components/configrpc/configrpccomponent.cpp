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

	m_ConfigRpcEndpoint = boost::make_shared<VirtualEndpoint>();

	long configSource;
	if (GetConfig()->GetProperty("configSource", &configSource) && configSource != 0) {
		m_ConfigRpcEndpoint->RegisterTopicHandler("config::FetchObjects",
		    boost::bind(&ConfigRpcComponent::FetchObjectsHandler, this, _2));

		ConfigObject::GetAllObjects()->OnObjectAdded.connect(boost::bind(&ConfigRpcComponent::LocalObjectCommittedHandler, this, _2));
		ConfigObject::GetAllObjects()->OnObjectCommitted.connect(boost::bind(&ConfigRpcComponent::LocalObjectCommittedHandler, this, _2));
		ConfigObject::GetAllObjects()->OnObjectRemoved.connect(boost::bind(&ConfigRpcComponent::LocalObjectRemovedHandler, this, _2));

		m_ConfigRpcEndpoint->RegisterPublication("config::ObjectCommitted");
		m_ConfigRpcEndpoint->RegisterPublication("config::ObjectRemoved");
	}

	endpointManager->OnNewEndpoint.connect(boost::bind(&ConfigRpcComponent::NewEndpointHandler, this, _2));

	m_ConfigRpcEndpoint->RegisterPublication("config::FetchObjects");
	m_ConfigRpcEndpoint->RegisterTopicHandler("config::ObjectCommitted",
	    boost::bind(&ConfigRpcComponent::RemoteObjectCommittedHandler, this, _3));
	m_ConfigRpcEndpoint->RegisterTopicHandler("config::ObjectRemoved",
	    boost::bind(&ConfigRpcComponent::RemoteObjectRemovedHandler, this, _3));

	endpointManager->RegisterEndpoint(m_ConfigRpcEndpoint);
}

void ConfigRpcComponent::Stop(void)
{
	EndpointManager::Ptr mgr = GetEndpointManager();

	if (mgr)
		mgr->UnregisterEndpoint(m_ConfigRpcEndpoint);
}

void ConfigRpcComponent::NewEndpointHandler(const Endpoint::Ptr& endpoint)
{
	/* no need to sync the config with local endpoints */
	if (endpoint->IsLocal())
		return;

	endpoint->OnSessionEstablished.connect(boost::bind(&ConfigRpcComponent::SessionEstablishedHandler, this, _1));
}

void ConfigRpcComponent::SessionEstablishedHandler(const Endpoint::Ptr& endpoint)
{
	RequestMessage request;
	request.SetMethod("config::FetchObjects");

	GetEndpointManager()->SendUnicastMessage(m_ConfigRpcEndpoint, endpoint, request);
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

void ConfigRpcComponent::FetchObjectsHandler(const Endpoint::Ptr& sender)
{
	ConfigObject::Set::Ptr allObjects = ConfigObject::GetAllObjects();

	for (ConfigObject::Set::Iterator ci = allObjects->Begin(); ci != allObjects->End(); ci++) {
		ConfigObject::Ptr object = *ci;

		if (!ShouldReplicateObject(object))
			continue;

		RequestMessage request = MakeObjectMessage(object, "config::ObjectCreated", true);

		GetEndpointManager()->SendUnicastMessage(m_ConfigRpcEndpoint, sender, request);
	}
}

void ConfigRpcComponent::LocalObjectCommittedHandler(const ConfigObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	GetEndpointManager()->SendMulticastMessage(m_ConfigRpcEndpoint,
	    MakeObjectMessage(object, "config::ObjectCreated", true));
}

void ConfigRpcComponent::LocalObjectRemovedHandler(const ConfigObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	GetEndpointManager()->SendMulticastMessage(m_ConfigRpcEndpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", false));
}

void ConfigRpcComponent::RemoteObjectCommittedHandler(const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string name;
	if (!params.GetProperty("name", &name))
		return;

	string type;
	if (!params.GetProperty("type", &type))
		return;

	MessagePart properties;
	if (!params.GetProperty("properties", &properties))
		return;

	ConfigObject::Ptr object = ConfigObject::GetObject(type, name);

	if (!object)
		object = boost::make_shared<ConfigObject>(properties.GetDictionary());
	else
		object->SetProperties(properties.GetDictionary());

	object->Commit();
}

void ConfigRpcComponent::RemoteObjectRemovedHandler(const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string name;
	if (!params.GetProperty("name", &name))
		return;

	string type;
	if (!params.GetProperty("type", &type))
		return;

	ConfigObject::Ptr object = ConfigObject::GetObject(type, name);

	if (!object)
		return;

	if (!object->IsLocal())
		object->Unregister();
}

EXPORT_COMPONENT(configrpc, ConfigRpcComponent);
