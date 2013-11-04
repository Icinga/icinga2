/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef SERVICEGROUP_H
#define SERVICEGROUP_H

#include "icinga/i2-icinga.h"
#include "icinga/servicegroup.th"
#include "icinga/service.h"

namespace icinga
{

/**
 * An Icinga service group.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API ServiceGroup : public ObjectImpl<ServiceGroup>
{
public:
	DECLARE_PTR_TYPEDEFS(ServiceGroup);
	DECLARE_TYPENAME(ServiceGroup);

	std::set<Service::Ptr> GetMembers(void) const;
	void AddMember(const Service::Ptr& service);
	void RemoveMember(const Service::Ptr& service);

private:
	std::set<Service::Ptr> m_Members;
};

}

#endif /* SERVICEGROUP_H */
