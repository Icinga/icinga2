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

#ifndef DISCOVERYCOMPONENT_H
#define DISCOVERYCOMPONENT_H

namespace icinga
{

/**
 * @ingroup discovery
 */
class ComponentDiscoveryInfo : public Object
{
public:
	typedef shared_ptr<ComponentDiscoveryInfo> Ptr;
	typedef weak_ptr<ComponentDiscoveryInfo> WeakPtr;

	string Node;
	string Service;

	set<string> Subscriptions;
	set<string> Publications;

	time_t LastSeen;
};

/**
 * @ingroup discovery
 */
class DiscoveryComponent : public IcingaComponent
{
public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);

private:
	VirtualEndpoint::Ptr m_DiscoveryEndpoint;
	map<string, ComponentDiscoveryInfo::Ptr> m_Components;
	Timer::Ptr m_DiscoveryTimer;

	void NewEndpointHandler(const Endpoint::Ptr& endpoint);
	void NewIdentityHandler(const Endpoint::Ptr& endpoint);

	void NewComponentMessageHandler(const RequestMessage& request);
	void RegisterComponentMessageHandler(const Endpoint::Ptr& sender, const RequestMessage& request);

	void WelcomeMessageHandler(const Endpoint::Ptr& sender, const RequestMessage& request);

	void SendDiscoveryMessage(const string& method, const string& identity, const Endpoint::Ptr& recipient);
	void ProcessDiscoveryMessage(const string& identity, const DiscoveryMessage& message, bool trusted);

	bool GetComponentDiscoveryInfo(string component, ComponentDiscoveryInfo::Ptr *info) const;

	void CheckExistingEndpoint(const Endpoint::Ptr& self, const Endpoint::Ptr& other);
	void DiscoveryEndpointHandler(const Endpoint::Ptr& endpoint, const ComponentDiscoveryInfo::Ptr& info) const;

	void DiscoveryTimerHandler(void);

	void FinishDiscoverySetup(const Endpoint::Ptr& endpoint);

	bool HasMessagePermission(const Dictionary::Ptr& roles, const string& messageType, const string& message);

	static const int RegistrationTTL = 300;
};

}

#endif /* DISCOVERYCOMPONENT_H */
