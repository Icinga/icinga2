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

#include "i2-cibsync.h"

using namespace icinga;

/**
 * Returns the name of the component.
 *
 * @returns The name.
 */
string CIBSyncComponent::GetName(void) const
{
	return "cibsync";
}

/**
 * Starts the component.
 */
void CIBSyncComponent::Start(void)
{
	m_SyncingConfig = false;

	m_Endpoint = boost::make_shared<VirtualEndpoint>();

	/* config objects */
	m_Endpoint->RegisterTopicHandler("config::FetchObjects",
	    boost::bind(&CIBSyncComponent::FetchObjectsHandler, this, _2));

	ConfigObject::GetAllObjects()->OnObjectAdded.connect(boost::bind(&CIBSyncComponent::LocalObjectCommittedHandler, this, _2));
	ConfigObject::GetAllObjects()->OnObjectCommitted.connect(boost::bind(&CIBSyncComponent::LocalObjectCommittedHandler, this, _2));
	ConfigObject::GetAllObjects()->OnObjectRemoved.connect(boost::bind(&CIBSyncComponent::LocalObjectRemovedHandler, this, _2));

	m_Endpoint->RegisterPublication("config::ObjectCommitted");
	m_Endpoint->RegisterPublication("config::ObjectRemoved");

	EndpointManager::GetInstance()->OnNewEndpoint.connect(boost::bind(&CIBSyncComponent::NewEndpointHandler, this, _2));

	m_Endpoint->RegisterPublication("config::FetchObjects");
	m_Endpoint->RegisterTopicHandler("config::ObjectCommitted",
	    boost::bind(&CIBSyncComponent::RemoteObjectCommittedHandler, this, _2, _3));
	m_Endpoint->RegisterTopicHandler("config::ObjectRemoved",
	    boost::bind(&CIBSyncComponent::RemoteObjectRemovedHandler, this, _3));

	/* service status */
	m_Endpoint->RegisterTopicHandler("delegation::ServiceStatus",
	    boost::bind(&CIBSyncComponent::ServiceStatusRequestHandler, _2, _3));

	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);
}

/**
 * Stops the component.
 */
void CIBSyncComponent::Stop(void)
{
	EndpointManager::Ptr endpointManager = EndpointManager::GetInstance();

	if (endpointManager)
		endpointManager->UnregisterEndpoint(m_Endpoint);
}

void CIBSyncComponent::ServiceStatusRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	ServiceStatusMessage params;
	if (!request.GetParams(&params))
		return;

	CIB::OnServiceStatusUpdate(params);

	string svcname;
	if (!params.GetService(&svcname))
		return;

	Service service = Service::GetByName(svcname);

	time_t nextCheck;
	if (params.GetNextCheck(&nextCheck))
		service.SetNextCheck(nextCheck);

	ServiceState state;
	ServiceStateType stateType;
	if (params.GetState(&state) && params.GetStateType(&stateType)) {
		ServiceState old_state = service.GetState();
		ServiceStateType old_stateType = service.GetStateType();

		if (state != old_state) {
			time_t now;
			time(&now);

			service.SetLastStateChange(now);

			if (old_stateType != stateType)
				service.SetLastHardStateChange(now);
		}

		service.SetState(state);
		service.SetStateType(stateType);
	}

	long attempt;
	if (params.GetCurrentCheckAttempt(&attempt))
		service.SetCurrentCheckAttempt(attempt);

	CheckResult cr;
	if (params.GetCheckResult(&cr))
		service.SetLastCheckResult(cr);

	time_t now;
	time(&now);
	CIB::UpdateTaskStatistics(now, 1);
}

void CIBSyncComponent::NewEndpointHandler(const Endpoint::Ptr& endpoint)
{
	/* no need to sync the config with local endpoints */
	if (endpoint->IsLocal())
		return;

	endpoint->OnSessionEstablished.connect(boost::bind(&CIBSyncComponent::SessionEstablishedHandler, this, _1));
}

void CIBSyncComponent::SessionEstablishedHandler(const Endpoint::Ptr& endpoint)
{
	RequestMessage request;
	request.SetMethod("config::FetchObjects");

	EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, endpoint, request);
}

RequestMessage CIBSyncComponent::MakeObjectMessage(const ConfigObject::Ptr& object, string method, bool includeProperties)
{
	RequestMessage msg;
	msg.SetMethod(method);

	MessagePart params;
	msg.SetParams(params);

	params.Set("name", object->GetName());
	params.Set("type", object->GetType());

	if (includeProperties)
		params.Set("properties", object->GetProperties());

	return msg;
}

bool CIBSyncComponent::ShouldReplicateObject(const ConfigObject::Ptr& object)
{
	return (!object->IsLocal());
}

void CIBSyncComponent::FetchObjectsHandler(const Endpoint::Ptr& sender)
{
	ConfigObject::Set::Ptr allObjects = ConfigObject::GetAllObjects();

	BOOST_FOREACH(const ConfigObject::Ptr& object, allObjects) {
		if (!ShouldReplicateObject(object))
			continue;

		RequestMessage request = MakeObjectMessage(object, "config::ObjectCommitted", true);

		EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, sender, request);
	}
}

void CIBSyncComponent::LocalObjectCommittedHandler(const ConfigObject::Ptr& object)
{
	/* don't send messages when we're currently processing a remote update */
	if (m_SyncingConfig)
		return;

	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectCommitted", true));
}

void CIBSyncComponent::LocalObjectRemovedHandler(const ConfigObject::Ptr& object)
{
	/* don't send messages when we're currently processing a remote update */
	if (m_SyncingConfig)
		return;

	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", false));
}

void CIBSyncComponent::RemoteObjectCommittedHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string name;
	if (!params.Get("name", &name))
		return;

	string type;
	if (!params.Get("type", &type))
		return;

	MessagePart properties;
	if (!params.Get("properties", &properties))
		return;

	ConfigObject::Ptr object = ConfigObject::GetObject(type, name);

	if (!object) {
		object = boost::make_shared<ConfigObject>(properties.GetDictionary());

		if (object->GetSource() == EndpointManager::GetInstance()->GetIdentity()) {
			/* the peer sent us an object that was originally created by us - 
			 * however if was deleted locally so we have to tell the peer to destroy
			 * its copy of the object. */
			EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
			    MakeObjectMessage(object, "config::ObjectRemoved", false));

			return;
		}
	} else {
		ConfigObject::Ptr remoteObject = boost::make_shared<ConfigObject>(properties.GetDictionary());

		if (object->GetCommitTimestamp() >= remoteObject->GetCommitTimestamp())
			return;

		object->SetProperties(properties.GetDictionary());
	}

	if (object->IsLocal())
		throw_exception(invalid_argument("Replicated remote object is marked as local."));

	if (object->GetSource().empty())
		object->SetSource(sender->GetIdentity());

	try {
		/* TODO: only ignore updates for _this_ object rather than all objects
		 * this might be relevant if the commit handler for this object
		 * creates other objects. */
		m_SyncingConfig = true;
		object->Commit();
		m_SyncingConfig = false;
	} catch (const exception&) {
		m_SyncingConfig = false;
		throw;
	}
}

void CIBSyncComponent::RemoteObjectRemovedHandler(const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string name;
	if (!params.Get("name", &name))
		return;

	string type;
	if (!params.Get("type", &type))
		return;

	ConfigObject::Ptr object = ConfigObject::GetObject(type, name);

	if (!object)
		return;

	if (!object->IsLocal()) {
		try {
			m_SyncingConfig = true;
			object->Unregister();
			m_SyncingConfig = false;
		} catch (const exception&) {
			m_SyncingConfig = false;
			throw;
		}
	}
}

EXPORT_COMPONENT(cibsync, CIBSyncComponent);
