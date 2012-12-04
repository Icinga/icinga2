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

namespace icinga
{

struct AttributeDescription
{
	String Name;
	DynamicAttributeType Type;
};

class I2_BASE_API DynamicType : public Object
{
public:
	typedef shared_ptr<DynamicType> Ptr;
	typedef weak_ptr<DynamicType> WeakPtr;

	typedef function<DynamicObject::Ptr (const Dictionary::Ptr&)> ObjectFactory;
	typedef map<String, DynamicType::Ptr, string_iless> TypeMap;
	typedef map<String, DynamicObject::Ptr, string_iless> NameMap;

	DynamicType(const String& name, const ObjectFactory& factory);

	String GetName(void) const;

	static DynamicType::Ptr GetByName(const String& name);

	static void RegisterType(const DynamicType::Ptr& type);
	static bool TypeExists(const String& name);
	
	DynamicObject::Ptr CreateObject(const Dictionary::Ptr& serializedUpdate) const;
	DynamicObject::Ptr GetObject(const String& name) const;

	void RegisterObject(const DynamicObject::Ptr& object);
	void UnregisterObject(const DynamicObject::Ptr& object);

	static TypeMap& GetTypes(void);
	NameMap& GetObjects(void);

	void AddAttribute(const String& name, DynamicAttributeType type);
	void RemoveAttribute(const String& name);

	void AddAttributes(const AttributeDescription *attributes, int attributeCount);

private:
	String m_Name;
	ObjectFactory m_ObjectFactory;
	map<String, DynamicAttributeType> m_Attributes;

	NameMap m_Objects;
};

/**
 * Helper class for registering DynamicObject implementation classes.
 *
 * @ingroup base
 */
class RegisterTypeHelper
{
public:
	RegisterTypeHelper(const String& name, const DynamicType::ObjectFactory& factory, const AttributeDescription* attributes, int attributeCount)
	{
		if (!DynamicType::TypeExists(name)) {
			DynamicType::Ptr type = boost::make_shared<DynamicType>(name, factory);
			type->AddAttributes(attributes, attributeCount);
			DynamicType::RegisterType(type);
		}
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

#define REGISTER_TYPE_ALIAS(type, alias, attributeDesc) \
	static RegisterTypeHelper g_Register ## type(alias, DynamicObjectFactory<type>, attributeDesc, (attributeDesc == NULL) ? 0 : sizeof(attributeDesc) / sizeof((static_cast<AttributeDescription *>(attributeDesc))[0]))

#define REGISTER_TYPE(type, attributeDesc) \
	REGISTER_TYPE_ALIAS(type, #type, attributeDesc)

}

#endif /* DYNAMICTYPE_H */
