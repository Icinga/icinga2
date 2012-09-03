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

namespace icinga
{

enum DynamicAttributeType
{
	Attribute_Transient = 1,

	/* Unlike transient attributes local attributes are persisted
	 * in the program state file. */
	Attribute_Local = 2,

	/* Replicated attributes are sent to other daemons for which
	 * replication is enabled. */
	Attribute_Replicated = 4,

	/* Attributes read from the config file are implicitly marked
	 * as config attributes. */
	Attribute_Config = 8,

	/* Combination of all attribute types */
	Attribute_All = Attribute_Transient | Attribute_Local | Attribute_Replicated | Attribute_Config
};

struct DynamicAttribute
{
	Value Data;
	DynamicAttributeType Type;
	double Tx;
};

/**
 * A dynamic object that can be instantiated from the configuration file.
 *
 * @ingroup base
 */
class I2_BASE_API DynamicObject : public Object
{
public:
	typedef shared_ptr<DynamicObject> Ptr;
	typedef weak_ptr<DynamicObject> WeakPtr;

	typedef function<DynamicObject::Ptr (const Dictionary::Ptr&)> Factory;

	typedef map<String, Factory> ClassMap;
	typedef map<String, DynamicObject::Ptr> NameMap;
	typedef map<String, NameMap> TypeMap;

	typedef map<String, DynamicAttribute> AttributeMap;
	typedef AttributeMap::iterator AttributeIterator;
	typedef AttributeMap::const_iterator AttributeConstIterator;

	DynamicObject(const Dictionary::Ptr& serializedObject);

	Dictionary::Ptr BuildUpdate(double sinceTx, int attributeTypes) const;
	void ApplyUpdate(const Dictionary::Ptr& serializedUpdate, int allowedTypes);

	void RegisterAttribute(const String& name, DynamicAttributeType type);

	void Set(const String& name, const Value& data);
	void Touch(const String& name);
	Value Get(const String& name) const;

	bool HasAttribute(const String& name) const;

	void ClearAttributesByType(DynamicAttributeType type);

	AttributeConstIterator AttributeBegin(void) const;
	AttributeConstIterator AttributeEnd(void) const;

	static boost::signal<void (const DynamicObject::Ptr&)> OnRegistered;
	static boost::signal<void (const DynamicObject::Ptr&)> OnUnregistered;
	static boost::signal<void (const set<DynamicObject::Ptr>&)> OnTransactionClosing;

	ScriptTask::Ptr InvokeMethod(const String& method,
	    const vector<Value>& arguments, ScriptTask::CompletionCallback callback);

	String GetType(void) const;
	String GetName(void) const;

	bool IsLocal(void) const;
	bool IsAbstract(void) const;

	void SetSource(const String& value);
	String GetSource(void) const;

	void SetTx(double tx);
	double GetTx(void) const;

	void Register(void);
	void Unregister(void);

	static DynamicObject::Ptr GetObject(const String& type, const String& name);
	static pair<TypeMap::iterator, TypeMap::iterator> GetTypes(void);
	static pair<NameMap::iterator, NameMap::iterator> GetObjects(const String& type);

	static void DumpObjects(const String& filename);
	static void RestoreObjects(const String& filename);
	static void DeactivateObjects(void);

	static void RegisterClass(const String& type, Factory factory);
	static bool ClassExists(const String& type);
	static DynamicObject::Ptr Create(const String& type, const Dictionary::Ptr& serializedUpdate);

	static double GetCurrentTx(void);
	static void BeginTx(void);
	static void FinishTx(void);

protected:
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	void InternalSetAttribute(const String& name, const Value& data, double tx, bool suppressEvent = false);
	Value InternalGetAttribute(const String& name) const;

	static ClassMap& GetClasses(void);
	static TypeMap& GetAllObjects(void);

	AttributeMap m_Attributes;
	double m_ConfigTx;

	static double m_CurrentTx;

	static set<DynamicObject::Ptr> m_ModifiedObjects;

	void InternalApplyUpdate(const Dictionary::Ptr& serializedUpdate, int allowedTypes, bool suppressEvents);
};

class RegisterClassHelper
{
public:
	RegisterClassHelper(const String& name, DynamicObject::Factory factory)
	{
		if (!DynamicObject::ClassExists(name))
			DynamicObject::RegisterClass(name, factory);
	}
};

template<typename T>
shared_ptr<T> DynamicObjectFactory(const Dictionary::Ptr& serializedUpdate)
{
	return boost::make_shared<T>(serializedUpdate);
}

#define REGISTER_CLASS_ALIAS(klass, alias) \
	static RegisterClassHelper g_Register ## klass(alias, DynamicObjectFactory<klass>)

#define REGISTER_CLASS(klass) \
	REGISTER_CLASS_ALIAS(klass, #klass)

}

#endif /* DYNAMICOBJECT_H */
