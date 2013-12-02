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

#ifndef DYNAMICTYPE_H
#define DYNAMICTYPE_H

#include "base/i2-base.h"
#include "base/registry.h"
#include "base/dynamicobject.h"
#include "base/debug.h"
#include <map>
#include <set>
#include <boost/function.hpp>

namespace icinga
{

class I2_BASE_API DynamicType : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DynamicType);

	DynamicType(const String& name);

	String GetName(void) const;

	static DynamicType::Ptr GetByName(const String& name);

	DynamicObject::Ptr CreateObject(const Dictionary::Ptr& serializedUpdate);
	DynamicObject::Ptr GetObject(const String& name) const;

	void RegisterObject(const DynamicObject::Ptr& object);

	static std::vector<DynamicType::Ptr> GetTypes(void);
	std::vector<DynamicObject::Ptr> GetObjects(void) const;

	template<typename T>
	static std::vector<shared_ptr<T> > GetObjects(void)
	{
		std::vector<shared_ptr<T> > objects;

		BOOST_FOREACH(const DynamicObject::Ptr& object, GetObjects(T::GetTypeName())) {
			shared_ptr<T> tobject = static_pointer_cast<T>(object);

			ASSERT(tobject);

			objects.push_back(tobject);
		}

		return objects;
	}

private:
	String m_Name;

	typedef std::map<String, DynamicObject::Ptr> ObjectMap;
	typedef std::vector<DynamicObject::Ptr> ObjectVector;

	ObjectMap m_ObjectMap;
	ObjectVector m_ObjectVector;

	typedef std::map<String, DynamicType::Ptr> TypeMap;
	typedef std::vector<DynamicType::Ptr> TypeVector;

	static TypeMap& InternalGetTypeMap(void);
	static TypeVector& InternalGetTypeVector(void);
	static boost::mutex& GetStaticMutex(void);

	static std::vector<DynamicObject::Ptr> GetObjects(const String& type);
};

}

#endif /* DYNAMICTYPE_H */
