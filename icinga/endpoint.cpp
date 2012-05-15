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

#include "i2-icinga.h"

using namespace icinga;

/**
 * Constructor for the Endpoint class.
 */
Endpoint::Endpoint(void)
{
	m_ReceivedWelcome = false;
	m_SentWelcome = false;
}

/**
 * Retrieves the identity of this endpoint.
 *
 * @returns The identity of the endpoint.
 */
string Endpoint::GetIdentity(void) const
{
	return m_Identity;
}

/**
 * Sets the identity of this endpoint.
 *
 * @param identity The new identity of the endpoint.
 */
void Endpoint::SetIdentity(string identity)
{
	m_Identity = identity;

	EventArgs ea;
	ea.Source = shared_from_this();
	OnIdentityChanged(ea);
}

/**
 * Retrieves the endpoint manager this endpoint is registered with.
 *
 * @returns The EndpointManager object.
 */
EndpointManager::Ptr Endpoint::GetEndpointManager(void) const
{
	return m_EndpointManager.lock();
}

/**
 * Sets the endpoint manager this endpoint is registered with.
 *
 * @param manager The EndpointManager object.
 */
void Endpoint::SetEndpointManager(EndpointManager::WeakPtr manager)
{
	m_EndpointManager = manager;
}

/**
 * Registers a topic subscription for this endpoint.
 *
 * @param topic The name of the topic.
 */
void Endpoint::RegisterSubscription(string topic)
{
	m_Subscriptions.insert(topic);
}

/**
 * Removes a topic subscription from this endpoint.
 *
 * @param topic The name of the topic.
 */
void Endpoint::UnregisterSubscription(string topic)
{
	m_Subscriptions.erase(topic);
}

/**
 * Checks whether the endpoint has a subscription for the specified topic.
 *
 * @param topic The name of the topic.
 * @returns true if the endpoint is subscribed to the topic, false otherwise.
 */
bool Endpoint::HasSubscription(string topic) const
{
	return (m_Subscriptions.find(topic) != m_Subscriptions.end());
}

/**
 * Registers a topic publication for this endpoint.
 *
 * @param topic The name of the topic.
 */
void Endpoint::RegisterPublication(string topic)
{
	m_Publications.insert(topic);
}

/**
 * Removes a topic publication from this endpoint.
 *
 * @param topic The name of the topic.
 */
void Endpoint::UnregisterPublication(string topic)
{
	m_Publications.erase(topic);
}

/**
 * Checks whether the endpoint  has a publication for the specified topic.
 *
 * @param topic The name of the topic.
 * @returns true if the endpoint is publishing this topic, false otherwise.
 */
bool Endpoint::HasPublication(string topic) const
{
	return (m_Publications.find(topic) != m_Publications.end());
}

/**
 * Removes all subscriptions for the endpoint.
 */
void Endpoint::ClearSubscriptions(void)
{
	m_Subscriptions.clear();
}

/**
 * Removes all publications for the endpoint.
 */
void Endpoint::ClearPublications(void)
{
	m_Publications.clear();
}

/**
 * Returns the beginning of the subscriptions list.
 *
 * @returns An iterator that points to the first subscription.
 */
Endpoint::ConstTopicIterator Endpoint::BeginSubscriptions(void) const
{
	return m_Subscriptions.begin();
}

/**
 * Returns the end of the subscriptions list.
 *
 * @returns An iterator that points past the last subscription.
 */
Endpoint::ConstTopicIterator Endpoint::EndSubscriptions(void) const
{
	return m_Subscriptions.end();
}

/**
 * Returns the beginning of the publications list.
 *
 * @returns An iterator that points to the first publication.
 */
Endpoint::ConstTopicIterator Endpoint::BeginPublications(void) const
{
	return m_Publications.begin();
}

/**
 * Returns the end of the publications list.
 *
 * @returns An iterator that points past the last publication.
 */
Endpoint::ConstTopicIterator Endpoint::EndPublications(void) const
{
	return m_Publications.end();
}

/**
 * Sets whether a welcome message has been received from this endpoint.
 *
 * @param value Whether we've received a welcome message.
 */
void Endpoint::SetReceivedWelcome(bool value)
{
	m_ReceivedWelcome = value;
}

/**
 * Retrieves whether a welcome message has been received from this endpoint.
 *
 * @returns Whether we've received a welcome message.
 */
bool Endpoint::GetReceivedWelcome(void) const
{
	return m_ReceivedWelcome;
}

/**
 * Sets whether a welcome message has been sent to this endpoint.
 *
 * @param value Whether we've sent a welcome message.
 */
void Endpoint::SetSentWelcome(bool value)
{
	m_SentWelcome = value;
}

/**
 * Retrieves whether a welcome message has been sent to this endpoint.
 *
 * @returns Whether we've sent a welcome message.
 */
bool Endpoint::GetSentWelcome(void) const
{
	return m_SentWelcome;
}
