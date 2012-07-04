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

#ifndef DELEGATIONCOMPONENT_H
#define DELEGATIONCOMPONENT_H

namespace icinga
{

/**
 * @ingroup delegation
 */
class DelegationComponent : public Component
{
public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);

private:
	VirtualEndpoint::Ptr m_Endpoint;
	ConfigObject::Set::Ptr m_AllServices;
	Timer::Ptr m_DelegationTimer;

	void NewEndpointHandler(const Endpoint::Ptr& endpoint);
	void SessionEstablishedHandler(const Endpoint::Ptr& endpoint);

	void ServiceCommittedHandler(Service service);
	void ServiceRemovedHandler(Service service);
	void DelegationTimerHandler(void);

	vector<Endpoint::Ptr> GetCheckerCandidates(const Service& service) const;

	void AssignService(const Endpoint::Ptr& checker, const Service& service);
	void ClearServices(const Endpoint::Ptr& checker);

	static bool IsEndpointChecker(const Endpoint::Ptr& endpoint);

	void CheckResultRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request);
};

}

#endif /* DELEGATIONCOMPONENT_H */
