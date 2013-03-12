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

#include "i2-replication.h"

using namespace icinga;

REGISTER_TYPE(ReplicationComponent);

ReplicationComponent::ReplicationComponent(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{ }

/**
 * Starts the component.
 */
void ReplicationComponent::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("replication", false);

	DynamicObject::OnRegistered.connect(boost::bind(&ReplicationComponent::LocalObjectRegisteredHandler, this, _1));
	DynamicObject::OnUnregistered.connect(boost::bind(&ReplicationComponent::LocalObjectUnregisteredHandler, this, _1));
	DynamicObject::OnTransactionClosing.connect(boost::bind(&ReplicationComponent::TransactionClosingHandler, this, _1, _2));
	DynamicObject::OnFlushObject.connect(boost::bind(&ReplicationComponent::FlushObjectHandler, this, _1, _2));

	Endpoint::OnConnected.connect(boost::bind(&ReplicationComponent::EndpointConnectedHandler, this, _1));

	m_Endpoint->RegisterTopicHandler("config::ObjectUpdate",
	    boost::bind(&ReplicationComponent::RemoteObjectUpdateHandler, this, _3));
	m_Endpoint->RegisterTopicHandler("config::ObjectRemoved",
	    boost::bind(&ReplicationComponent::RemoteObjectRemovedHandler, this, _3));

	/* service status */
	m_Endpoint->RegisterTopicHandler("checker::CheckResult",
	    boost::bind(&ReplicationComponent::CheckResultRequestHandler, _3));
}

/**
 * Stops the component.
 */
void ReplicationComponent::Stop(void)
{
	m_Endpoint->Unregister();
}

void ReplicationComponent::CheckResultRequestHandler(const RequestMessage& request)
{
	CheckResultMessage params;
	if (!request.GetParams(&params))
		return;

	String svcname = params.GetService();
	Service::Ptr service = Service::GetByName(svcname);

	Dictionary::Ptr cr = params.GetCheckResult();
	if (!cr)
		return;

	if (cr->Contains("current_checker") && cr->Get("current_checker") == EndpointManager::GetInstance()->GetIdentity())
		return;

	Service::UpdateStatistics(cr);
}

void ReplicationComponent::EndpointConnectedHandler(const Endpoint::Ptr& endpoint)
{
	{
		ObjectLock olock(endpoint);

		/* no need to sync the config with local endpoints */
		if (endpoint->IsLocalEndpoint())
			return;

		/* we just assume the other endpoint wants object updates */
		endpoint->RegisterSubscription("config::ObjectUpdate");
		endpoint->RegisterSubscription("config::ObjectRemoved");
	}

	DynamicType::Ptr type;
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		set<DynamicObject::Ptr> objects;

		{
			ObjectLock olock(dt);
			objects = dt->GetObjects();
		}

		BOOST_FOREACH(const DynamicObject::Ptr& object, objects) {
			if (!ShouldReplicateObject(object))
				continue;

			RequestMessage request = MakeObjectMessage(object, "config::ObjectUpdate", 0, true);

			EndpointManager::Ptr em = EndpointManager::GetInstance();

			{
				ObjectLock elock(em);
				em->SendUnicastMessage(m_Endpoint, endpoint, request);
			}

		}
	}
}

RequestMessage ReplicationComponent::MakeObjectMessage(const DynamicObject::Ptr& object, const String& method, double sinceTx, bool includeProperties)
{
	RequestMessage msg;
	msg.SetMethod(method);

	MessagePart params;
	msg.SetParams(params);

	params.Set("name", object->GetName());
	params.Set("type", object->GetType());

	String source = object->GetSource();

	if (source.IsEmpty())
		source = EndpointManager::GetInstance()->GetIdentity();

	params.Set("source", source);

	if (includeProperties)
		params.Set("update", object->BuildUpdate(sinceTx, Attribute_Replicated | Attribute_Config));

	return msg;
}

bool ReplicationComponent::ShouldReplicateObject(const DynamicObject::Ptr& object)
{
	return (!object->IsLocal());
}

void ReplicationComponent::LocalObjectRegisteredHandler(const DynamicObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectUpdate", 0, true));
}

void ReplicationComponent::LocalObjectUnregisteredHandler(const DynamicObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
	    MakeObjectMessage(object, "config::ObjectRemoved", 0, false));
}

void ReplicationComponent::TransactionClosingHandler(double tx, const set<DynamicObject::WeakPtr>& modifiedObjects)
{
	if (modifiedObjects.empty())
		return;

	stringstream msgbuf;
	msgbuf << "Sending " << modifiedObjects.size() << " replication updates.";
	Logger::Write(LogDebug, "replication", msgbuf.str());

	BOOST_FOREACH(const DynamicObject::WeakPtr& wobject, modifiedObjects) {
		DynamicObject::Ptr object = wobject.lock();

		if (!object)
			continue;

		FlushObjectHandler(tx, object);
	}
}

void ReplicationComponent::FlushObjectHandler(double tx, const DynamicObject::Ptr& object)
{
	if (!ShouldReplicateObject(object))
		return;

	RequestMessage request = MakeObjectMessage(object, "config::ObjectUpdate", tx, true);

	EndpointManager::Ptr em = EndpointManager::GetInstance();
	{
		ObjectLock olock(em);
		em->SendMulticastMessage(m_Endpoint, request);
	}
}

void ReplicationComponent::RemoteObjectUpdateHandler(const RequestMessage& request)
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

	String source;
	if (!params.Get("source", &source))
		return;

	Dictionary::Ptr update;
	if (!params.Get("update", &update))
		return;

	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	DynamicObject::Ptr object = dtype->GetObject(name);

	// TODO: sanitize update, disallow __local

	if (!object) {
		object = dtype->CreateObject(update);

		if (source == EndpointManager::GetInstance()->GetIdentity()) {
			/* the peer sent us an object that was originally created by us -
			 * however it was deleted locally so we have to tell the peer to destroy
			 * its copy of the object. */
			EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint,
			    MakeObjectMessage(object, "config::ObjectRemoved", 0, false));

			return;
		}

		Logger::Write(LogDebug, "replication", "Received object from source: " + source);

		object->SetSource(source);
		object->Register();
	} else {
		if (object->IsLocal())
			BOOST_THROW_EXCEPTION(invalid_argument("Replicated remote object is marked as local."));

		// TODO: disallow config updates depending on endpoint config

		object->ApplyUpdate(update, Attribute_All);
	}
}

void ReplicationComponent::RemoteObjectRemovedHandler(const RequestMessage& request)
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
