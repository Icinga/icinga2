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

#ifndef HOSTGROUP_H
#define HOSTGROUP_H

namespace icinga
{

/**
 * An Icinga host group.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API HostGroup : public DynamicObject
{
public:
	typedef shared_ptr<HostGroup> Ptr;
	typedef weak_ptr<HostGroup> WeakPtr;

	HostGroup(const Dictionary::Ptr& properties)
		: DynamicObject(properties)
	{ }

	static bool Exists(const String& name);
	static HostGroup::Ptr GetByName(const String& name);

	String GetAlias(void) const;
	String GetNotesUrl(void) const;
	String GetActionUrl(void) const;

	set<Host::Ptr> GetMembers(void) const;
	static void InvalidateMembersCache(void);

private:
	static map<String, vector<weak_ptr<Host> > > m_MembersCache;
	static bool m_MembersCacheValid;

	static void ValidateMembersCache(void);
};

}

#endif /* HOSTGROUP_H */
