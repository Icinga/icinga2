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

#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include <map>

namespace icinga
{

class ConfigHive;

class I2_BASE_API ConfigObject : public Dictionary
{
private:
	weak_ptr<ConfigHive> m_Hive;

	string m_Name;
	string m_Type;
	bool m_Replicated;

public:
	typedef shared_ptr<ConfigObject> Ptr;
	typedef weak_ptr<ConfigObject> WeakPtr;

	ConfigObject(const string& type, const string& name);

	void SetHive(const weak_ptr<ConfigHive>& hive);
	weak_ptr<ConfigHive> GetHive(void) const;

	void SetName(const string& name);
	string GetName(void) const;

	void SetType(const string& type);
	string GetType(void) const;

	void SetReplicated(bool replicated);
	bool GetReplicated(void) const;

	void Commit(void);
};

}

#endif /* CONFIGOBJECT_H */
