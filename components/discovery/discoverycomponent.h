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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef DISCOVERYCOMPONENT_H
#define DISCOVERYCOMPONENT_H

namespace icinga
{

class ComponentDiscoveryInfo : public Object
{
public:
	typedef shared_ptr<ComponentDiscoveryInfo> Ptr;
	typedef weak_ptr<ComponentDiscoveryInfo> WeakPtr;

	string Node;
	string Service;

	set<string> SubscribedMethods;
	set<string> PublishedMethods;

	time_t LastSeen;
};

class DiscoveryComponent : public IcingaComponent
{
private:
	VirtualEndpoint::Ptr m_DiscoveryEndpoint;
	map<string, ComponentDiscoveryInfo::Ptr> m_Components;
	bool m_Broker;
	Timer::Ptr m_DiscoveryTimer;

	int NewEndpointHandler(const NewEndpointEventArgs& neea);
	int NewIdentityHandler(const EventArgs& ea);

	int NewComponentMessageHandler(const NewRequestEventArgs& nrea);
	int RegisterComponentMessageHandler(const NewRequestEventArgs& nrea);

	int WelcomeMessageHandler(const NewRequestEventArgs& nrea);

	void SendDiscoveryMessage(string method, string identity, Endpoint::Ptr recipient);
	void ProcessDiscoveryMessage(string identity, DiscoveryMessage message, bool trusted);

	bool GetComponentDiscoveryInfo(string component, ComponentDiscoveryInfo::Ptr *info) const;

	int CheckExistingEndpoint(Endpoint::Ptr endpoint, const NewEndpointEventArgs& neea);
	int DiscoveryEndpointHandler(const NewEndpointEventArgs& neea, ComponentDiscoveryInfo::Ptr info) const;
	int DiscoverySinkHandler(const NewMethodEventArgs& nmea, ComponentDiscoveryInfo::Ptr info) const;
	int DiscoverySourceHandler(const NewMethodEventArgs& nmea, ComponentDiscoveryInfo::Ptr info) const;

	int DiscoveryTimerHandler(const TimerEventArgs& tea);

	bool IsBroker(void) const;

	void FinishDiscoverySetup(Endpoint::Ptr endpoint);

	int EndpointConfigHandler(const EventArgs& ea);

	bool HasMessagePermission(Dictionary::Ptr roles, string messageType, string message);

	static const int RegistrationTTL = 300;

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* DISCOVERYCOMPONENT_H */
