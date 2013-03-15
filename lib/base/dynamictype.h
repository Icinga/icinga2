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

#ifndef DYNAMICTYPE_H
#define DYNAMICTYPE_H

#include <boost/function.hpp>

namespace icinga
{

class I2_BASE_API DynamicType : public Object
{
public:
	typedef shared_ptr<DynamicType> Ptr;
	typedef weak_ptr<DynamicType> WeakPtr;

	typedef boost::function<DynamicObject::Ptr (const Dictionary::Ptr&)> ObjectFactory;

	DynamicType(const String& name, const ObjectFactory& factory);

	String GetName(void) const;

	static DynamicType::Ptr GetByName(const String& name);

	static void RegisterType(const DynamicType::Ptr& type);

	DynamicObject::Ptr CreateObject(const Dictionary::Ptr& serializedUpdate);
	DynamicObject::Ptr GetObject(const String& name) const;

	void RegisterObject(const DynamicObject::Ptr& object);
	void UnregisterObject(const DynamicObject::Ptr& object);

	static set<DynamicType::Ptr> GetTypes(void);
	set<DynamicObject::Ptr> GetObjects(void) const;

	static set<DynamicObject::Ptr> GetObjects(const String& type);

private:
	String m_Name;
	ObjectFactory m_ObjectFactory;

	typedef map<String, DynamicObject::Ptr, string_iless> ObjectMap;
	typedef set<DynamicObject::Ptr> ObjectSet;

	ObjectMap m_ObjectMap;
	ObjectSet m_ObjectSet;

	typedef map<String, DynamicType::Ptr, string_iless> TypeMap;
	typedef set<DynamicType::Ptr> TypeSet;

	static TypeMap& InternalGetTypeMap(void);
	static TypeSet& InternalGetTypeSet(void);
	static boost::mutex& GetStaticMutex(void);
};

/**
 * A registry for DynamicType objects.
 *
 * @ingroup base
 */
class DynamicTypeRegistry : public Registry<DynamicType::Ptr>
{ };

/**
 * Helper class for registering DynamicObject implementation classes.
 *
 * @ingroup base
 */
class RegisterTypeHelper
{
public:
	RegisterTypeHelper(const String& name, const DynamicType::ObjectFactory& factory)
	{
		DynamicType::Ptr type = boost::make_shared<DynamicType>(name, factory);
		DynamicType::RegisterType(type);
	}
};

/**
 * Factory function for DynamicObject-based classes.
 *
 * @ingroup base
 */
template<typename T>
shared_ptr<T> DynamicObjectFactory(const Dictionary::Ptr& serializedUpdate)
{
	return boost::make_shared<T>(serializedUpdate);
}

#define REGISTER_TYPE_ALIAS(type, alias) \
	I2_EXPORT icinga::RegisterTypeHelper g_RegisterDT_ ## type(alias, DynamicObjectFactory<type>);

#define REGISTER_TYPE(type) \
	REGISTER_TYPE_ALIAS(type, #type)

}

#endif /* DYNAMICTYPE_H */
