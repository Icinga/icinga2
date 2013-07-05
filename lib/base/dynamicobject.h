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

#ifndef DYNAMICOBJECT_H
#define DYNAMICOBJECT_H

#include "base/i2-base.h"
#include "base/attribute.h"
#include "base/object.h"
#include "base/dictionary.h"
#include <boost/signals2.hpp>
#include <map>
#include <set>

namespace icinga
{

class DynamicType;

/**
 * A dynamic object that can be instantiated from the configuration file
 * and that supports attribute replication to remote application instances.
 *
 * @ingroup base
 */
class I2_BASE_API DynamicObject : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DynamicObject);

	typedef std::map<String, AttributeHolder, string_iless> AttributeMap;
	typedef AttributeMap::iterator AttributeIterator;
	typedef AttributeMap::const_iterator AttributeConstIterator;

	~DynamicObject(void);

	static void Initialize(void);

	Dictionary::Ptr BuildUpdate(double sinceTx, int attributeTypes) const;
	void ApplyUpdate(const Dictionary::Ptr& serializedUpdate, int allowedTypes);

	void RegisterAttribute(const String& name, AttributeType type, AttributeBase *boundAttribute = NULL);

	void Set(const String& name, const Value& data);
	void Touch(const String& name);
	Value Get(const String& name) const;

	void BindAttribute(const String& name, Value *boundValue);

	void ClearAttributesByType(AttributeType type);

	static boost::signals2::signal<void (const DynamicObject::Ptr&)> OnRegistered;
	static boost::signals2::signal<void (const DynamicObject::Ptr&)> OnUnregistered;
	static boost::signals2::signal<void (double, const std::set<DynamicObject::WeakPtr>&)> OnTransactionClosing;
	static boost::signals2::signal<void (double, const DynamicObject::Ptr&)> OnFlushObject;

	Value InvokeMethod(const String& method, const std::vector<Value>& arguments);

	shared_ptr<DynamicType> GetType(void) const;
	String GetName(void) const;

	bool IsLocal(void) const;
	bool IsRegistered(void) const;

	void SetSource(const String& value);
	String GetSource(void) const;

	void SetExtension(const String& key, const Object::Ptr& object);
	Object::Ptr GetExtension(const String& key);
	void ClearExtension(const String& key);

	void Flush(void);

	void Register(void);
	void Unregister(void);

	virtual void Start(void);
	virtual void Stop(void);

	double GetLocalTx(void) const;

	const AttributeMap& GetAttributes(void) const;

	static DynamicObject::Ptr GetObject(const String& type, const String& name);

	static void DumpObjects(const String& filename);
	static void RestoreObjects(const String& filename);
	static void DeactivateObjects(void);

	static double GetCurrentTx(void);

	Dictionary::Ptr GetCustom(void) const;

protected:
	explicit DynamicObject(const Dictionary::Ptr& serializedObject);

	virtual void OnRegistrationCompleted(void);
	virtual void OnUnregistrationCompleted(void);

	virtual void OnAttributeChanged(const String& name);

private:
	void InternalSetAttribute(const String& name, const Value& data, double tx, bool allowEditConfig = false);
	Value InternalGetAttribute(const String& name) const;
	void InternalRegisterAttribute(const String& name, AttributeType type, AttributeBase *boundAttribute = NULL);

	mutable boost::mutex m_AttributeMutex;
	AttributeMap m_Attributes;
	std::set<String, string_iless> m_ModifiedAttributes;
	double m_ConfigTx;
	double m_LocalTx;

	Attribute<String> m_Name;
	Attribute<String> m_Type;
	Attribute<bool> m_Local;
	Attribute<String> m_Source;
	Attribute<Dictionary::Ptr> m_Extensions;
	Attribute<Dictionary::Ptr> m_Methods;
	Attribute<Dictionary::Ptr> m_Custom;

	bool m_Registered;	/**< protected by the type mutex */

	static double m_CurrentTx;

	static void NewTx(void);

	friend class DynamicType; /* for OnRegistrationCompleted. */
};

}

#endif /* DYNAMICOBJECT_H */
