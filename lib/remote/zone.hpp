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

#ifndef ZONE_H
#define ZONE_H

#include "remote/i2-remote.hpp"
#include "remote/zone.thpp"
#include "remote/endpoint.hpp"

namespace icinga
{

/**
 * @ingroup remote
 */
class Zone final : public ObjectImpl<Zone>
{
public:
	DECLARE_OBJECT(Zone);
	DECLARE_OBJECTNAME(Zone);

	virtual void OnAllConfigLoaded() override;

	Zone::Ptr GetParent() const;
	std::set<Endpoint::Ptr> GetEndpoints() const;
	std::vector<Zone::Ptr> GetAllParents() const;

	bool CanAccessObject(const ConfigObject::Ptr& object);
	bool IsChildOf(const Zone::Ptr& zone);
	bool IsGlobal() const;
	bool IsSingleInstance() const;

	static Zone::Ptr GetLocalZone();

protected:
	virtual void ValidateEndpointsRaw(const Array::Ptr& value, const ValidationUtils& utils) override;

private:
	Zone::Ptr m_Parent;
	std::vector<Zone::Ptr> m_AllParents;
};

}

#endif /* ZONE_H */
