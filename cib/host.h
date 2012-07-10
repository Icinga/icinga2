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

#ifndef HOST_H
#define HOST_H

namespace icinga
{

class I2_CIB_API Host : public ConfigObjectAdapter
{
public:
	Host(const ConfigObject::Ptr& configObject);

	static bool Exists(const string& name);
	static Host GetByName(const string& name);

	string GetAlias(void) const;
	Dictionary::Ptr GetGroups(void) const;
	set<string> GetParents(void) const;

	bool IsReachable(void) const;
	bool IsUp(void) const;
};

}

#endif /* HOST_H */
