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

#ifndef CONFIGCOLLECTION_H
#define CONFIGCOLLECTION_H

namespace icinga
{

class ConfigHive;

/**
 * A collection of configuration objects that each have the same type.
 */
class I2_BASE_API ConfigCollection : public Object
{
private:
	weak_ptr<ConfigHive> m_Hive;

public:
	typedef shared_ptr<ConfigCollection> Ptr;
	typedef weak_ptr<ConfigCollection> WeakPtr;

	typedef map<string, ConfigObject::Ptr>::iterator ObjectIterator;
	typedef map<string, ConfigObject::Ptr>::const_iterator ObjectConstIterator;
	map<string, ConfigObject::Ptr> Objects;

	void SetHive(const weak_ptr<ConfigHive>& hive);
	weak_ptr<ConfigHive> GetHive(void) const;

	void AddObject(const ConfigObject::Ptr& object);
	void RemoveObject(const ConfigObject::Ptr& object);
	ConfigObject::Ptr GetObject(const string& name = string()) const;

	void ForEachObject(function<int (const EventArgs&)> callback);

	Observable<EventArgs> OnObjectCommitted;
	Observable<EventArgs> OnObjectRemoved;
};

}

#endif /* CONFIGCOLLECTION_H */
