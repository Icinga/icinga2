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

#ifndef CONFIGRPCCOMPONENT_H
#define CONFIGRPCCOMPONENT_H

namespace icinga
{

/**
 * @ingroup configrpc
 */
class ConfigRpcComponent : public IcingaComponent
{
private:
	VirtualEndpoint::Ptr m_ConfigRpcEndpoint;

	int NewEndpointHandler(const NewEndpointEventArgs& ea);
	int SessionEstablishedHandler(const EventArgs& ea);

	int LocalObjectCommittedHandler(const EventArgs& ea);
	int LocalObjectRemovedHandler(const EventArgs& ea);

	int FetchObjectsHandler(const NewRequestEventArgs& ea);
	int RemoteObjectCommittedHandler(const NewRequestEventArgs& ea);
	int RemoteObjectRemovedHandler(const NewRequestEventArgs& ea);

	static RpcRequest MakeObjectMessage(const ConfigObject::Ptr& object,
	    string method, bool includeProperties);

	static bool ShouldReplicateObject(const ConfigObject::Ptr& object);
public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* CONFIGRPCCOMPONENT_H */
