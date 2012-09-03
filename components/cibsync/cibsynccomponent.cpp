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
	m_Endpoint = Endpoint::MakeEndpoint("cibsync", true);

	DynamicObject::OnRegistered.connect(boost::bind(&CIBSyncComponent::LocalObjectRegisteredHandler, this, _1));
	DynamicObject::OnUnregistered.connect(boost::bind(&CIBSyncComponent::LocalObjectUnregisteredHandler, this, _1));
	DynamicObject::OnTransactionClosing.connect(boost::bind(&CIBSyncComponent::TransactionClosingHandler, this, _1));

	Endpoint::OnConnected.connect(boost::bind(&CIBSyncComponent::EndpointConnectedHandler, this, _1));
	
	m_Endpoint->RegisterTopicHandler("config::ObjectUpdate",
	    boost::bind(&CIBSyncComponent::RemoteObjectUpdateHandler, this, _2, _3));
	m_Endpoint->RegisterTopicHandler("config::ObjectRemoved",
	    boost::bind(&CIBSyncComponent::RemoteObjectRemovedHandler, this, _3));

	/* service status */
	m_Endpoint->RegisterTopicHandler("checker::ServiceStateChange",
	    boost::bind(&CIBSyncComponent::ServiceStateChangeRequestHandler, _2, _3));
}

/**
 * Stops the component.
 */
void CIBSyncComponent::Stop(void)
{
	m_Endpoint->Unregister();
}

void CIBSyncComponent::ServiceStateChangeRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	ServiceStateChangeMessage params;
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

	time_t now = static_cast<time_t>(Utility::GetTime());
	CIB::UpdateTaskStatistics(now, 1);
}

void CIBSyncComponent::EndpointConnectedHandler(const Endpoint::Ptr& endpoint)
{
	/* no need to sync the config with local endpoints */
	if (endpoint->IsLocalEndpoint())
		return;

	/* we just assume the other endpoint wants object updates */
	endpoint->RegisterSubscription("config::ObjectUpdate");
	endpoint->RegisterSubscription("config::ObjectRemoved");

	pair<DynamicObject::TypeMap::iterator, DynamicObject::TypeMap::iterator> trange = DynamicObject::GetTypes();
	DynamicObject::TypeMap::iterator tt;
	for (tt = trange.first; tt != trange.second; tt++) {
		DynamicObject::Ptr object;
		BOOST_FOREACH(tie(tuples::ignore, object), tt->second) {
			if (!ShouldReplicateObject(object))
				continue;

			RequestMessage request = MakeObjectMessage(object, "config::ObjectUpdate", 0, true);
			EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, endpoint, request);
		}
	}
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

void CIBSyncComponent::LocalObjectRegisteredHandler(const DynamicObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectUpdate", 0, true));
}

void CIBSyncComponent::LocalObjectUnregisteredHandler(const DynamicObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", 0, false));
}

void CIBSyncComponent::TransactionClosingHandler(const set<DynamicObject::Ptr>& modifiedObjects)
{
	if (modifiedObjects.empty())
		return;

	stringstream msgbuf;
	msgbuf << "Sending " << modifiedObjects.size() << " replication updates.";
	Logger::Write(LogDebug, "cibsync", msgbuf.str());

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

	// TODO: sanitize update, disallow __local

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

		if (object->GetSource().IsEmpty())
			object->SetSource(sender->GetName());

		object->Register();
	} else {
		if (object->IsLocal())
			throw_exception(invalid_argument("Replicated remote object is marked as local."));

		// TODO: disallow config updates depending on endpoint config

		object->ApplyUpdate(update, Attribute_All);
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
		object->Unregister();
	}
}

EXPORT_COMPONENT(cibsync, CIBSyncComponent);
