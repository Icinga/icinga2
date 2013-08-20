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

#include "remoting/endpoint.h"
#include "remoting/endpointmanager.h"
#include "remoting/jsonrpc.h"
#include "base/application.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include "config/configitembuilder.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_TYPE(Endpoint);

boost::signals2::signal<void (const Endpoint::Ptr&)> Endpoint::OnConnected;

/**
 * Helper function for creating new endpoint objects.
 *
 * @param name The name of the new endpoint.
 * @param replicated Whether replication is enabled for the endpoint object.
 * @param local Whether the new endpoint should be local.
 * @returns The new endpoint.
 */
Endpoint::Ptr Endpoint::MakeEndpoint(const String& name, bool replicated, bool local)
{
	ConfigItemBuilder::Ptr endpointConfig = boost::make_shared<ConfigItemBuilder>();
	endpointConfig->SetType("Endpoint");
	endpointConfig->SetName((!replicated && local) ? "local:" + name : name);
	//TODO: endpointConfig->SetLocal(!replicated);
	endpointConfig->AddExpression("local", OperatorSet, local);

	ConfigItem::Ptr item = endpointConfig->Compile();
	DynamicObject::Ptr object = item->Commit();
	return dynamic_pointer_cast<Endpoint>(object);
}

/**
 * Checks whether this is a local endpoint.
 *
 * @returns true if this is a local endpoint, false otherwise.
 */
bool Endpoint::IsLocalEndpoint(void) const
{
	return m_Local;
}

/**
 * Checks whether this endpoint is connected.
 *
 * @returns true if the endpoint is connected, false otherwise.
 */
bool Endpoint::IsConnected(void) const
{
	if (IsLocalEndpoint()) {
		return true;
	} else {
		return GetClient();
	}
}

Stream::Ptr Endpoint::GetClient(void) const
{
	ObjectLock olock(this);

	return m_Client;
}

void Endpoint::SetClient(const Stream::Ptr& client)
{
	{
		ObjectLock olock(this);

		m_Client = client;
	}

	boost::thread thread(boost::bind(&Endpoint::MessageThreadProc, this, client));
	thread.detach();

	OnConnected(GetSelf());
}

/**
 * Registers a topic subscription for this endpoint.
 *
 * @param topic The name of the topic.
 */
void Endpoint::RegisterSubscription(const String& topic)
{
	Dictionary::Ptr subscriptions = GetSubscriptions();

	if (!subscriptions)
		subscriptions = boost::make_shared<Dictionary>();

	if (!subscriptions->Contains(topic)) {
		Dictionary::Ptr newSubscriptions = subscriptions->ShallowClone();
		newSubscriptions->Set(topic, topic);

		ObjectLock olock(this);
		SetSubscriptions(newSubscriptions);
	}
}

/**
 * Removes a topic subscription from this endpoint.
 *
 * @param topic The name of the topic.
 */
void Endpoint::UnregisterSubscription(const String& topic)
{
	Dictionary::Ptr subscriptions = GetSubscriptions();

	if (!subscriptions)
		return;

	if (subscriptions->Contains(topic)) {
		Dictionary::Ptr newSubscriptions = subscriptions->ShallowClone();
		newSubscriptions->Remove(topic);
		SetSubscriptions(newSubscriptions);
	}
}

/**
 * Checks whether the endpoint has a subscription for the specified topic.
 *
 * @param topic The name of the topic.
 * @returns true if the endpoint is subscribed to the topic, false otherwise.
 */
bool Endpoint::HasSubscription(const String& topic) const
{
	Dictionary::Ptr subscriptions = GetSubscriptions();

	return (subscriptions && subscriptions->Contains(topic));
}

/**
 * Removes all subscriptions for the endpoint.
 */
void Endpoint::ClearSubscriptions(void)
{
	m_Subscriptions = Empty;
}

Dictionary::Ptr Endpoint::GetSubscriptions(void) const
{
	return m_Subscriptions;
}

void Endpoint::SetSubscriptions(const Dictionary::Ptr& subscriptions)
{
	subscriptions->Seal();
	m_Subscriptions = subscriptions;
}

void Endpoint::RegisterTopicHandler(const String& topic, const boost::function<Endpoint::Callback>& callback)
{
	ObjectLock olock(this);

	std::map<String, shared_ptr<boost::signals2::signal<Endpoint::Callback> > >::iterator it;
	it = m_TopicHandlers.find(topic);

	shared_ptr<boost::signals2::signal<Endpoint::Callback> > sig;

	if (it == m_TopicHandlers.end()) {
		sig = boost::make_shared<boost::signals2::signal<Endpoint::Callback> >();
		m_TopicHandlers.insert(make_pair(topic, sig));
	} else {
		sig = it->second;
	}

	sig->connect(callback);

	olock.Unlock();

	RegisterSubscription(topic);
}

void Endpoint::ProcessRequest(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	if (!IsConnected()) {
		// TODO: persist the message
		return;
	}

	if (IsLocalEndpoint()) {
		ObjectLock olock(this);

		String method;
		if (!request.GetMethod(&method))
			return;

		std::map<String, shared_ptr<boost::signals2::signal<Endpoint::Callback> > >::iterator it;
		it = m_TopicHandlers.find(method);

		if (it == m_TopicHandlers.end())
			return;

		Utility::QueueAsyncCallback(boost::bind(boost::ref(*it->second), GetSelf(), sender, request));
	} else {
		try {
			JsonRpc::SendMessage(GetClient(), request);
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Error while sending JSON-RPC message for endpoint '" << GetName() << "': " << boost::diagnostic_information(ex);
			Log(LogWarning, "remoting", msgbuf.str());

			m_Client.reset();
		}
	}
}

void Endpoint::ProcessResponse(const Endpoint::Ptr& sender, const ResponseMessage& response)
{
	if (!IsConnected())
		return;

	if (IsLocalEndpoint())
		EndpointManager::GetInstance()->ProcessResponseMessage(sender, response);
	else {
		try {
			JsonRpc::SendMessage(GetClient(), response);
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Error while sending JSON-RPC message for endpoint '" << GetName() << "': " << boost::diagnostic_information(ex);
			Log(LogWarning, "remoting", msgbuf.str());

			m_Client.reset();
		}
	}
}

void Endpoint::MessageThreadProc(const Stream::Ptr& stream)
{
	for (;;) {
		MessagePart message;

		try {
			message = JsonRpc::ReadMessage(stream);
		} catch (const std::exception& ex) {
			Log(LogWarning, "jsonrpc", "Error while reading JSON-RPC message for endpoint '" + GetName() + "': " + boost::diagnostic_information(ex));

			m_Client.reset();
		}

		Endpoint::Ptr sender = GetSelf();

		if (ResponseMessage::IsResponseMessage(message)) {
			/* rather than routing the message to the right virtual
			 * endpoint we just process it here right away. */
			EndpointManager::GetInstance()->ProcessResponseMessage(sender, message);
			return;
		}

		RequestMessage request = message;

		String method;
		if (!request.GetMethod(&method))
			return;

		String id;
		if (request.GetID(&id))
			EndpointManager::GetInstance()->SendAnycastMessage(sender, request);
		else
			EndpointManager::GetInstance()->SendMulticastMessage(sender, request);
	}
}

/**
 * Gets the node address for this endpoint.
 *
 * @returns The node address (hostname).
 */
String Endpoint::GetNode(void) const
{
	ObjectLock olock(this);

	return m_Node;
}

/**
 * Gets the service name for this endpoint.
 *
 * @returns The service name (port).
 */
String Endpoint::GetService(void) const
{
	ObjectLock olock(this);

	return m_Service;
}

void Endpoint::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("local", m_Local);
		bag->Set("node", m_Node);
		bag->Set("service", m_Service);
	}

	bag->Set("subscriptions", m_Subscriptions);
}

void Endpoint::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_Local = bag->Get("local");
		m_Node = bag->Get("node");
		m_Service = bag->Get("service");
	}

	bag->Set("subscriptions", m_Subscriptions);
}