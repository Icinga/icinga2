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

/**
 * The type of an attribute for a DynamicObject.
 *
 * @ingroup base
 */
enum AttributeType
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

class AttributeBase
{
public:
	AttributeBase(void)
		: m_Value()
	{ }

	void InternalSet(const Value& value)
	{
		m_Value = value;
	}

	const Value& InternalGet(void) const
	{
		return m_Value;
	}

	operator Value(void) const
	{
		return InternalGet();
	}

	bool IsEmpty(void) const
	{
		return InternalGet().IsEmpty();
	}

private:
	Value m_Value;
};

template<typename T>
class Attribute : public AttributeBase
{
public:
	void Set(const T& value)
	{
		InternalSet(value);
	}

	Attribute<T>& operator=(const T& rhs)
	{
		Set(rhs);
		return *this;
	}

	T Get(void) const
	{
		if (IsEmpty())
			return T();

		return InternalGet();
	}

	operator T(void) const
	{
		return Get();
	}
};

/**
 * An attribute for a DynamicObject.
 *
 * @ingroup base
 */
struct AttributeHolder
{
	AttributeType m_Type; /**< The type of the attribute. */
	double m_Tx; /**< The timestamp of the last value change. */
	bool m_OwnsAttribute; /**< Whether we own the Data pointer. */
	AttributeBase *m_Attribute; /**< The current value of the attribute. */

	AttributeHolder(AttributeType type, AttributeBase *boundAttribute = NULL)
		: m_Type(type), m_Tx(0)
	{
		if (boundAttribute) {
			m_Attribute = boundAttribute;
			m_OwnsAttribute = false;
		} else {
			m_Attribute = new Attribute<Value>();
			m_OwnsAttribute = true;
		}
	}

	AttributeHolder(const AttributeHolder& other)
	{
		m_Type = other.m_Type;
		m_Tx = other.m_Tx;
		m_OwnsAttribute = other.m_OwnsAttribute;

		if (other.m_OwnsAttribute) {
			m_Attribute = new Attribute<Value>();
			m_Attribute->InternalSet(other.m_Attribute->InternalGet());
		} else {
			m_Attribute = other.m_Attribute;
		}
	}

	~AttributeHolder(void)
	{
		if (m_OwnsAttribute)
			delete m_Attribute;
	}

	void Bind(AttributeBase *boundAttribute)
	{
		assert(m_OwnsAttribute);
		boundAttribute->InternalSet(m_Attribute->InternalGet());
		m_Attribute = boundAttribute;
		m_OwnsAttribute = false;
	}

	void SetValue(double tx, const Value& value)
	{
		m_Tx = tx;
		m_Attribute->InternalSet(value);
	}

	Value GetValue(void) const
	{
		return m_Attribute->InternalGet();
	}

	void SetType(AttributeType type)
	{
		m_Type = type;
	}

	AttributeType GetType(void) const
	{
		return m_Type;
	}

	double GetTx(void) const
	{
		return m_Tx;
	}
};

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
	typedef shared_ptr<DynamicObject> Ptr;
	typedef weak_ptr<DynamicObject> WeakPtr;

	typedef map<String, AttributeHolder, string_iless> AttributeMap;
	typedef AttributeMap::iterator AttributeIterator;
	typedef AttributeMap::const_iterator AttributeConstIterator;

	DynamicObject(const Dictionary::Ptr& serializedObject);
	~DynamicObject(void);

	static void Initialize(void);

	Dictionary::Ptr BuildUpdate(double sinceTx, int attributeTypes) const;
	void ApplyUpdate(const Dictionary::Ptr& serializedUpdate, int allowedTypes);

	void RegisterAttribute(const String& name, AttributeType type, AttributeBase *boundAttribute = NULL);

	void Set(const String& name, const Value& data);
	void Touch(const String& name);
	Value Get(const String& name) const;

	bool HasAttribute(const String& name) const;

	void BindAttribute(const String& name, Value *boundValue);

	void ClearAttributesByType(AttributeType type);

	static signals2::signal<void (const DynamicObject::Ptr&)> OnRegistered;
	static signals2::signal<void (const DynamicObject::Ptr&)> OnUnregistered;
	static signals2::signal<void (double, const set<DynamicObject::WeakPtr>&)> OnTransactionClosing;
	static signals2::signal<void (double, const DynamicObject::Ptr&)> OnFlushObject;

	ScriptTask::Ptr MakeMethodTask(const String& method,
	    const vector<Value>& arguments);

	shared_ptr<DynamicType> GetType(void) const;
	String GetName(void) const;

	bool IsLocal(void) const;
	bool IsAbstract(void) const;
	bool IsRegistered(void) const;

	void SetSource(const String& value);
	String GetSource(void) const;

	void Flush(void);

	void Register(void);
	void Unregister(void);

	virtual void Start(void);

	const AttributeMap& GetAttributes(void) const;

	void SetEventSafe(bool initialized);
	bool GetEventSafe(void) const;

	static DynamicObject::Ptr GetObject(const String& type, const String& name);

	static void DumpObjects(const String& filename);
	static void RestoreObjects(const String& filename);
	static void DeactivateObjects(void);

	static double GetCurrentTx(void);

protected:
	virtual void OnRegistrationCompleted(void);
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	void InternalSetAttribute(const String& name, const Value& data, double tx, bool allowEditConfig = false);
	Value InternalGetAttribute(const String& name) const;
	void SendLocalUpdateEvents(void);

	AttributeMap m_Attributes;
	map<String, Value, string_iless> m_ModifiedAttributes;
	double m_ConfigTx;

	Attribute<String> m_Name;
	Attribute<String> m_Type;
	Attribute<bool> m_Local;
	Attribute<bool> m_Abstract;
	Attribute<String> m_Source;
	Attribute<Dictionary::Ptr> m_Methods;

	bool m_Registered;
	bool m_EventSafe;

	static double m_CurrentTx;

	static void NewTx(void);

	/* This has to be a set of raw pointers because the DynamicObject
	 * constructor has to be able to insert objects into this list. */
	static set<DynamicObject::WeakPtr> m_ModifiedObjects;
	static boost::mutex m_TransactionMutex;
	static boost::once_flag m_TransactionOnce;
	static Timer::Ptr m_TransactionTimer;

	friend class DynamicType; /* for OnRegistrationCompleted. */
};

}

#endif /* DYNAMICOBJECT_H */
