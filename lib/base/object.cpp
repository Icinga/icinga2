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

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
{
#ifdef _DEBUG
	boost::mutex::scoped_lock lock(GetMutex());
	GetAliveObjects().insert(this);
#endif /* _DEBUG */
}

/**
 * Destructor for the Object class.
 */
Object::~Object(void)
{
#ifdef _DEBUG
	boost::mutex::scoped_lock lock(GetMutex());
	GetAliveObjects().erase(this);
#endif /* _DEBUG */
}

/**
 * Temporarily holds onto a reference for an object. This can
 * be used to safely clear the last reference to an object
 * in an event handler.
 */
void Object::Hold(void)
{
	boost::mutex::scoped_lock lock(GetMutex());
	GetHeldObjects().push_back(GetSelf());
}

/**
 * Clears all temporarily held objects.
 */
void Object::ClearHeldObjects(void)
{
	boost::mutex::scoped_lock lock(GetMutex());
	GetHeldObjects().clear();
}

/**
 * Returns a reference-counted pointer to this object.
 *
 * @returns A shared_ptr object that points to this object
 */
Object::SharedPtrHolder Object::GetSelf(void)
{
	return Object::SharedPtrHolder(shared_from_this());
}

#ifdef _DEBUG
/**
 * Retrieves the number of currently alive objects.
 *
 * @returns The number of alive objects.
 */
int Object::GetAliveObjectsCount(void)
{
	boost::mutex::scoped_lock lock(GetMutex());
	return GetAliveObjects().size();
}

/**
 * Dumps a memory histogram to the "dictionaries.dump" file.
 */
void Object::PrintMemoryProfile(void)
{
	map<String, int> types;

	ofstream dictfp("dictionaries.dump.tmp");

	{
		boost::mutex::scoped_lock lock(GetMutex());
		set<Object *>::iterator it;
		BOOST_FOREACH(Object *obj, GetAliveObjects()) {
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

#ifdef _WIN32
	_unlink("dictionaries.dump");
#endif /* _WIN32 */

	dictfp.close();
	if (rename("dictionaries.dump.tmp", "dictionaries.dump") < 0)
		throw_exception(PosixException("rename() failed", errno));

	String type;
	int count;
	BOOST_FOREACH(tie(type, count), types) {
		std::cerr << type << ": " << count << std::endl;
	}
}

/**
 * Returns currently active objects.
 *
 * @returns currently active objects
 */
set<Object *>& Object::GetAliveObjects(void)
{
	static set<Object *> aliveObjects;
	return aliveObjects;
}
#endif /* _DEBUG */

/**
 * Returns the mutex used for accessing static members.
 *
 * @returns a mutex
 */
boost::mutex& Object::GetMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}

/**
 * Returns currently held objects. The caller must be
 * holding the mutex returned by GetMutex().
 *
 * @returns currently held objects
 */
vector<Object::Ptr>& Object::GetHeldObjects(void)
{
	static vector<Object::Ptr> heldObjects;
	return heldObjects;
}


