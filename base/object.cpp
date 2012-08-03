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

#include "i2-base.h"

using namespace icinga;

mutex Object::m_Mutex;
vector<Object::Ptr> Object::m_HeldObjects;
#ifdef _DEBUG
set<Object *> Object::m_AliveObjects;
#endif /* _DEBUG */

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
{
#ifdef _DEBUG
	mutex::scoped_lock lock(m_Mutex);
	m_AliveObjects.insert(this);
#endif /* _DEBUG */
}

/**
 * Destructor for the Object class.
 */
Object::~Object(void)
{
#ifdef _DEBUG
	mutex::scoped_lock lock(m_Mutex);
	m_AliveObjects.erase(this);
#endif /* _DEBUG */
}

/**
 * Temporarily holds onto a reference for an object. This can
 * be used to safely clear the last reference to an object
 * in an event handler.
 */
void Object::Hold(void)
{
	mutex::scoped_lock lock(m_Mutex);
	m_HeldObjects.push_back(GetSelf());
}

/**
 * Clears all temporarily held objects.
 */
void Object::ClearHeldObjects(void)
{
	mutex::scoped_lock lock(m_Mutex);
	m_HeldObjects.clear();
}

Object::SharedPtrHolder Object::GetSelf(void)
{
	return Object::SharedPtrHolder(shared_from_this());
}

#ifdef _DEBUG
int Object::GetAliveObjects(void)
{
	mutex::scoped_lock lock(m_Mutex);
	return m_AliveObjects.size();
}

void Object::PrintMemoryProfile(void)
{
	map<String, int> types;

	ofstream dictfp("dictionaries.dump.tmp");

	{
		mutex::scoped_lock lock(m_Mutex);
		set<Object *>::iterator it;
		BOOST_FOREACH(Object *obj, m_AliveObjects) {
			pair<map<String, int>::iterator, bool> tt;
			tt = types.insert(make_pair(Utility::GetTypeName(typeid(*obj)), 1));
			if (!tt.second)
				tt.first->second++;

			if (typeid(*obj) == typeid(Dictionary)) {
				Dictionary::Ptr dict = obj->GetSelf();
				dictfp << Value(dict).Serialize() << std::endl;
			}
		}
	}

	dictfp.close();
	rename("dictionaries.dump.tmp", "dictionaries.dump");

	String type;
	int count;
	BOOST_FOREACH(tie(type, count), types) {
		std::cerr << type << ": " << count << std::endl;
	}
}
#endif /* _DEBUG */
