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

string VirtualEndpoint::GetAddress(void) const
{
	char address[50];
	sprintf(address, "virtual:%p", (void *)this);
	return address;
}

bool VirtualEndpoint::IsLocal(void) const
{
	return true;
}

bool VirtualEndpoint::IsConnected(void) const
{
	return true;
}

void VirtualEndpoint::RegisterTopicHandler(string topic, function<void (const VirtualEndpoint::Ptr&, const Endpoint::Ptr, const RequestMessage&)> callback)
{
	map<string, shared_ptr<boost::signal<void (const VirtualEndpoint::Ptr&, const Endpoint::Ptr, const RequestMessage&)> > >::iterator it;
	it = m_TopicHandlers.find(topic);

	shared_ptr<boost::signal<void (const VirtualEndpoint::Ptr&, const Endpoint::Ptr, const RequestMessage&)> > sig;

	if (it == m_TopicHandlers.end()) {
		sig = boost::make_shared<boost::signal<void (const VirtualEndpoint::Ptr&, const Endpoint::Ptr, const RequestMessage&)> >();
		m_TopicHandlers.insert(make_pair(topic, sig));
	} else {
		sig = it->second;
	}

	sig->connect(callback);

	RegisterSubscription(topic);
}

void VirtualEndpoint::UnregisterTopicHandler(string topic, function<void (const VirtualEndpoint::Ptr&, const Endpoint::Ptr, const RequestMessage&)> callback)
{
	// TODO: implement
	//m_TopicHandlers[method] -= callback;
	//UnregisterMethodSubscription(method);

	throw NotImplementedException();
}

void VirtualEndpoint::ProcessRequest(Endpoint::Ptr sender, const RequestMessage& request)
{
	string method;
	if (!request.GetMethod(&method))
		return;

	map<string, shared_ptr<boost::signal<void (const VirtualEndpoint::Ptr&, const Endpoint::Ptr, const RequestMessage&)> > >::iterator it;
	it = m_TopicHandlers.find(method);

	if (it == m_TopicHandlers.end())
		return;

	(*it->second)(GetSelf(), sender, request);
}

void VirtualEndpoint::ProcessResponse(Endpoint::Ptr sender, const ResponseMessage& response)
{
	GetEndpointManager()->ProcessResponseMessage(sender, response);
}

void VirtualEndpoint::Stop(void)
{
	/* Nothing to do here. */
}
