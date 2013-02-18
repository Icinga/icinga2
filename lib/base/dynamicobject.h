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

/**
 * An attribute for a DynamicObject.
 *
 * @ingroup base
 */
struct DynamicAttribute
{
	Value Data; /**< The current value of the attribute. */
	DynamicAttributeType Type; /**< The type of the attribute. */
	double Tx; /**< The timestamp of the last value change. */
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

	typedef map<String, DynamicAttribute, string_iless> AttributeMap;
	typedef AttributeMap::iterator AttributeIterator;
	typedef AttributeMap::const_iterator AttributeConstIterator;

	DynamicObject(const Dictionary::Ptr& serializedObject);
	~DynamicObject(void);

	static void Initialize(void);

	Dictionary::Ptr BuildUpdate(double sinceTx, int attributeTypes) const;
	void ApplyUpdate(const Dictionary::Ptr& serializedUpdate, int allowedTypes);

	void RegisterAttribute(const String& name, DynamicAttributeType type);

	void Set(const String& name, const Value& data);
	void Touch(const String& name);
	Value Get(const String& name) const;

	bool HasAttribute(const String& name) const;

	void ClearAttributesByType(DynamicAttributeType type);

	static signals2::signal<void (const DynamicObject::Ptr&)> OnRegistered;
	static signals2::signal<void (const DynamicObject::Ptr&)> OnUnregistered;
	static signals2::signal<void (double, const set<DynamicObject *>&)> OnTransactionClosing;

	ScriptTask::Ptr InvokeMethod(const String& method,
	    const vector<Value>& arguments, ScriptTask::CompletionCallback callback);

	shared_ptr<DynamicType> GetType(void) const;
	String GetName(void) const;

	bool IsLocal(void) const;
	bool IsAbstract(void) const;

	void SetSource(const String& value);
	String GetSource(void) const;

	void SetTx(double tx);
	double GetTx(void) const;

	void Register(void);
	void Unregister(void);

	virtual void Start(void);

	const AttributeMap& GetAttributes(void) const;

	void SetEvents(bool events);
	bool GetEvents(void) const;

	static DynamicObject::Ptr GetObject(const String& type, const String& name);

	static void DumpObjects(const String& filename);
	static void RestoreObjects(const String& filename);
	static void DeactivateObjects(void);

	static double GetCurrentTx(void);
	static void NewTx(void);

protected:
	virtual void OnInitCompleted(void);
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	void InternalSetAttribute(const String& name, const Value& data, double tx, bool allowEditConfig = false);
	Value InternalGetAttribute(const String& name) const;
	void SendLocalUpdateEvents(void);

	AttributeMap m_Attributes;
	map<String, Value, string_iless> m_ModifiedAttributes;
	double m_ConfigTx;

	bool m_Events;

	static double m_CurrentTx;

	/* This has to be a set of raw pointers because the DynamicObject
	 * constructor has to be able to insert objects into this list. */
	static set<DynamicObject *> m_ModifiedObjects;
	static boost::mutex m_TransactionMutex;
	static boost::once_flag m_TransactionOnce;
	static Timer::Ptr m_TransactionTimer;

	friend class DynamicType; /* for OnInitCompleted. */
};

}

#endif /* DYNAMICOBJECT_H */
