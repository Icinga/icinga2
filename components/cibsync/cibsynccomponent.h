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

#ifndef CIBSYNCCOMPONENT_H
#define CIBSYNCCOMPONENT_H

namespace icinga
{

/**
 * @ingroup cibsync
 */
class CIBSyncComponent : public IComponent
{
public:
	virtual void Start(void);
	virtual void Stop(void);

private:
	VirtualEndpoint::Ptr m_Endpoint;

	static void ServiceStateChangeRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request);

	void NewEndpointHandler(const Endpoint::Ptr& endpoint);
	void SessionEstablishedHandler(const Endpoint::Ptr& endpoint);

	void LocalObjectRegisteredHandler(const DynamicObject::Ptr& object);
	void LocalObjectUnregisteredHandler(const DynamicObject::Ptr& object);
	void TransactionClosingHandler(const set<DynamicObject::Ptr>& modifiedObjects);

	void FetchObjectsHandler(const Endpoint::Ptr& sender);
	void RemoteObjectUpdateHandler(const Endpoint::Ptr& sender, const RequestMessage& request);
	void RemoteObjectRemovedHandler(const RequestMessage& request);

	static RequestMessage MakeObjectMessage(const DynamicObject::Ptr& object,
	    const String& method, double sinceTx, bool includeProperties);

	static bool ShouldReplicateObject(const DynamicObject::Ptr& object);
};

}

#endif /* CIBSYNCCOMPONENT_H */
