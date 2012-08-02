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
 * Starts the component.
 */
void CIBSyncComponent::Start(void)
{
	m_SyncingConfig = false;

	m_Endpoint = boost::make_shared<VirtualEndpoint>();

	/* config objects */
	m_Endpoint->RegisterTopicHandler("config::FetchObjects",
	    boost::bind(&CIBSyncComponent::FetchObjectsHandler, this, _2));

	DynamicObject::OnRegistered.connect(boost::bind(&CIBSyncComponent::LocalObjectRegisteredHandler, this, _1));
	DynamicObject::OnUnregistered.connect(boost::bind(&CIBSyncComponent::LocalObjectUnregisteredHandler, this, _1));
	DynamicObject::OnTransactionClosing.connect(boost::bind(&CIBSyncComponent::TransactionClosingHandler, this, _1));

	m_Endpoint->RegisterPublication("config::ObjectUpdate");
	m_Endpoint->RegisterPublication("config::ObjectRemoved");

	EndpointManager::GetInstance()->OnNewEndpoint.connect(boost::bind(&CIBSyncComponent::NewEndpointHandler, this, _2));

	m_Endpoint->RegisterPublication("config::FetchObjects");
	m_Endpoint->RegisterTopicHandler("config::ObjectUpdate",
	    boost::bind(&CIBSyncComponent::RemoteObjectUpdateHandler, this, _2, _3));
	m_Endpoint->RegisterTopicHandler("config::ObjectRemoved",
	    boost::bind(&CIBSyncComponent::RemoteObjectRemovedHandler, this, _3));

	/* service status */
	m_Endpoint->RegisterTopicHandler("checker::CheckResult",
	    boost::bind(&CIBSyncComponent::CheckResultRequestHandler, _2, _3));

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

void CIBSyncComponent::CheckResultRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	CheckResultMessage params;
	if (!request.GetParams(&params))
		return;

	String svcname;
	if (!params.GetService(&svcname))
		return;

	Service::Ptr service = Service::GetByName(svcname);

	//CheckResult cr;
	//if (!params.GetCheckResult(&cr))
	//	return;

	//Service::OnCheckResultReceived(service, params);
	//service->ApplyCheckResult(cr);

	time_t now = Utility::GetTime();
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

RequestMessage CIBSyncComponent::MakeObjectMessage(const DynamicObject::Ptr& object, const String& method, double sinceTx, bool includeProperties)
{
	RequestMessage msg;
	msg.SetMethod(method);

	MessagePart params;
	msg.SetParams(params);

	params.Set("name", object->GetName());
	params.Set("type", object->GetType());

	if (includeProperties)
		params.Set("update", object->BuildUpdate(sinceTx, Attribute_Replicated | Attribute_Config));

	return msg;
}

bool CIBSyncComponent::ShouldReplicateObject(const DynamicObject::Ptr& object)
{
	return (!object->IsLocal());
}

void CIBSyncComponent::FetchObjectsHandler(const Endpoint::Ptr& sender)
{
	pair<DynamicObject::TypeMap::iterator, DynamicObject::TypeMap::iterator> trange;
	DynamicObject::TypeMap::iterator tt;
	for (tt = trange.first; tt != trange.second; tt++) {
		DynamicObject::Ptr object;
		BOOST_FOREACH(tie(tuples::ignore, object), tt->second) {
			if (!ShouldReplicateObject(object))
				continue;

			RequestMessage request = MakeObjectMessage(object, "config::ObjectUpdate", 0, true);

			EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, sender, request);
		}
	}
}

void CIBSyncComponent::LocalObjectRegisteredHandler(const DynamicObject::Ptr& object)
{
	/* don't send messages when we're currently processing a remote update */
	if (m_SyncingConfig)
		return;

	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectCommitted", 0, true));
}

void CIBSyncComponent::LocalObjectUnregisteredHandler(const DynamicObject::Ptr& object)
{
	/* don't send messages when we're currently processing a remote update */
	if (m_SyncingConfig)
		return;

	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", 0, false));
}

void CIBSyncComponent::TransactionClosingHandler(const set<DynamicObject::Ptr>& modifiedObjects)
{
	stringstream msgbuf;
	msgbuf << "Sending " << modifiedObjects.size() << " replication updates.";
	Logger::Write(LogInformation, "cibsync", msgbuf.str());

	BOOST_FOREACH(const DynamicObject::Ptr& object, modifiedObjects) {
		if (!ShouldReplicateObject(object))
				continue;

		RequestMessage request = MakeObjectMessage(object, "config::ObjectUpdate", DynamicObject::GetCurrentTx(), true);

		EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint, request);
	}
}

void CIBSyncComponent::RemoteObjectUpdateHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	String name;
	if (!params.Get("name", &name))
		return;

	String type;
	if (!params.Get("type", &type))
		return;

	Dictionary::Ptr update;
	if (!params.Get("update", &update))
		return;

	DynamicObject::Ptr object = DynamicObject::GetObject(type, name);

	if (!object) {
		object = DynamicObject::Create(type, update);

		if (object->GetSource() == EndpointManager::GetInstance()->GetIdentity()) {
			/* the peer sent us an object that was originally created by us - 
			 * however it was deleted locally so we have to tell the peer to destroy
			 * its copy of the object. */
			EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
			    MakeObjectMessage(object, "config::ObjectRemoved", 0, false));

			return;
		}
	} else {
		if (object->IsLocal())
			throw_exception(invalid_argument("Replicated remote object is marked as local."));

		if (object->GetSource().IsEmpty())
			object->SetSource(sender->GetIdentity());

		object->ApplyUpdate(update, true);
	}
}

void CIBSyncComponent::RemoteObjectRemovedHandler(const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	String name;
	if (!params.Get("name", &name))
		return;

	String type;
	if (!params.Get("type", &type))
		return;

	DynamicObject::Ptr object = DynamicObject::GetObject(type, name);

	if (!object)
		return;

	if (!object->IsLocal()) {
		try {
			m_SyncingConfig = true;
			object->Unregister();
			m_SyncingConfig = false;
		} catch (...) {
			m_SyncingConfig = false;
			throw;
		}
	}
}

EXPORT_COMPONENT(cibsync, CIBSyncComponent);
