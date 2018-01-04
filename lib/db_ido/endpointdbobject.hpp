/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef ENDPOINTDBOBJECT_H
#define ENDPOINTDBOBJECT_H

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"
#include "remote/endpoint.hpp"

namespace icinga
{

/**
 * A Command database object.
 *
 * @ingroup ido
 */
class EndpointDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(EndpointDbObject);

	EndpointDbObject(const intrusive_ptr<DbType>& type, const String& name1, const String& name2);

	static void StaticInitialize();

	virtual Dictionary::Ptr GetConfigFields() const override;
	virtual Dictionary::Ptr GetStatusFields() const override;

private:
	static void UpdateConnectedStatus(const Endpoint::Ptr& endpoint);
	static int EndpointIsConnected(const Endpoint::Ptr& endpoint);
};

}

#endif /* ENDPOINTDBOBJECT_H */
