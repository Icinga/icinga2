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

double DynamicObject::m_CurrentTx = 0;
set<DynamicObject::WeakPtr> DynamicObject::m_ModifiedObjects;
boost::mutex DynamicObject::m_TransactionMutex;
boost::once_flag DynamicObject::m_TransactionOnce = BOOST_ONCE_INIT;
Timer::Ptr DynamicObject::m_TransactionTimer;

signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnRegistered;
signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnUnregistered;
signals2::signal<void (double, const set<DynamicObject::WeakPtr>&)> DynamicObject::OnTransactionClosing;
signals2::signal<void (double, const DynamicObject::Ptr&)> DynamicObject::OnFlushObject;

DynamicObject::DynamicObject(const Dictionary::Ptr& serializedObject)
	: m_EventSafe(false), m_ConfigTx(0), m_Registered(false)
{
	RegisterAttribute("__name", Attribute_Config, &m_Name);
	RegisterAttribute("__type", Attribute_Config, &m_Type);
	RegisterAttribute("__local", Attribute_Config, &m_Local);
	RegisterAttribute("__abstract", Attribute_Config, &m_Abstract);
	RegisterAttribute("__source", Attribute_Local, &m_Source);
	RegisterAttribute("methods", Attribute_Config, &m_Methods);

	if (!serializedObject->Contains("configTx"))
		BOOST_THROW_EXCEPTION(invalid_argument("Serialized object must contain a config snapshot."));

	/* apply config state from the config item/remote update;
	 * The DynamicType::CreateObject function takes care of restoring
	 * non-config state after the object has been fully constructed */
	ApplyUpdate(serializedObject, Attribute_Config);

	boost::call_once(m_TransactionOnce, &DynamicObject::Initialize);
}

/*
 * @threadsafety Always.
 */
DynamicObject::~DynamicObject(void)
{ }

void DynamicObject::Initialize(void)
{
	/* Set up a timer to periodically create a new transaction. */
	m_TransactionTimer = boost::make_shared<Timer>();
	m_TransactionTimer->SetInterval(0.5);
	m_TransactionTimer->OnTimerExpired.connect(boost::bind(&DynamicObject::NewTx));
	m_TransactionTimer->Start();
}

Dictionary::Ptr DynamicObject::BuildUpdate(double sinceTx, int attributeTypes) const
{
	assert(OwnsLock());

	DynamicObject::AttributeConstIterator it;

	Dictionary::Ptr attrs = boost::make_shared<Dictionary>();

	for (it = m_Attributes.begin(); it != m_Attributes.end(); it++) {
		if (it->second.GetType() == Attribute_Transient)
			continue;

		if ((it->second.GetType() & attributeTypes) == 0)
			continue;

		if (it->second.GetTx() == 0)
			continue;

		if (it->second.GetTx() < sinceTx && !(it->second.GetType() == Attribute_Config && m_ConfigTx >= sinceTx))
			continue;

		Dictionary::Ptr attr = boost::make_shared<Dictionary>();
		attr->Set("data", it->second.GetValue());
		attr->Set("type", it->second.GetType());
		attr->Set("tx", it->second.GetTx());

		attrs->Set(it->first, attr);
	}

	attrs->Seal();

	Dictionary::Ptr update = boost::make_shared<Dictionary>();
	update->Set("attrs", attrs);

	if (m_ConfigTx >= sinceTx && attributeTypes & Attribute_Config)
		update->Set("configTx", m_ConfigTx);
	else if (attrs->GetLength() == 0)
		return Dictionary::Ptr();

	update->Seal();

	return update;
}

void DynamicObject::ApplyUpdate(const Dictionary::Ptr& serializedUpdate,
    int allowedTypes)
{
	assert(OwnsLock());
	assert(serializedUpdate->IsSealed());

	Value configTxValue = serializedUpdate->Get("configTx");

	if ((allowedTypes & Attribute_Config) != 0 && !configTxValue.IsEmpty()) {
		double configTx = configTxValue;

		if (configTx > m_ConfigTx)
			ClearAttributesByType(Attribute_Config);
	}

	Dictionary::Ptr attrs = serializedUpdate->Get("attrs");

	assert(attrs->IsSealed());

	{
		ObjectLock olock(attrs);

		Dictionary::Iterator it;
		for (it = attrs->Begin(); it != attrs->End(); it++) {
			if (!it->second.IsObjectType<Dictionary>())
				continue;

			Dictionary::Ptr attr = it->second;

			assert(attr->IsSealed());

			int type = attr->Get("type");

			if ((type & ~allowedTypes) != 0)
				continue;

			Value data = attr->Get("data");
			double tx = attr->Get("tx");

			if (type & Attribute_Config)
				RegisterAttribute(it->first, Attribute_Config);

			if (!HasAttribute(it->first))
				RegisterAttribute(it->first, static_cast<AttributeType>(type));

			InternalSetAttribute(it->first, data, tx, true);
		}
	}
}

void DynamicObject::RegisterAttribute(const String& name,
    AttributeType type, AttributeBase *boundAttribute)
{
	assert(OwnsLock());

	AttributeHolder attr(type, boundAttribute);

	pair<DynamicObject::AttributeIterator, bool> tt;
	tt = m_Attributes.insert(make_pair(name, attr));

	if (!tt.second) {
		tt.first->second.SetType(type);

		if (boundAttribute)
			tt.first->second.Bind(boundAttribute);
	}
}

/**
 * @threadsafety Always.
 */
void DynamicObject::Set(const String& name, const Value& data)
{
	InternalSetAttribute(name, data, GetCurrentTx());
}

/**
 * @threadsafety Always.
 */
void DynamicObject::Touch(const String& name)
{
	InternalSetAttribute(name, InternalGetAttribute(name), GetCurrentTx());
}

/**
 * @threadsafety Always.
 */
Value DynamicObject::Get(const String& name) const
{
	return InternalGetAttribute(name);
}

/**
 * @threadsafety Always.
 */
void DynamicObject::InternalSetAttribute(const String& name, const Value& data,
    double tx, bool allowEditConfig)
{
	ObjectLock olock(this);

	DynamicObject::AttributeIterator it;
	it = m_Attributes.find(name);

	Value oldValue;

	if (it == m_Attributes.end()) {
		AttributeHolder attr(Attribute_Transient);
		attr.SetValue(tx, data);

		m_Attributes.insert(make_pair(name, attr));
	} else {
		if (!allowEditConfig && (it->second.GetType() & Attribute_Config))
			BOOST_THROW_EXCEPTION(runtime_error("Config properties are immutable: '" + name + "'."));

		oldValue = it->second.GetValue();
		it->second.SetValue(tx, data);

		if (it->second.GetType() & Attribute_Config)
			m_ConfigTx = tx;
	}

	if (GetEventSafe()) {
		/* We can't call GetSelf() in the constructor or destructor.
		 * The Register() function will take care of adding this
		 * object to the list of modified objects later on if we can't
		 * do it here. */

		DynamicObject::Ptr self = GetSelf();

		{
			boost::mutex::scoped_lock lock(m_TransactionMutex);
			m_ModifiedObjects.insert(self);
		}
	}

	/* Use insert() rather than [] so we don't overwrite
	 * an existing oldValue if the attribute was previously
	 * changed in the same transaction */
	m_ModifiedAttributes.insert(make_pair(name, oldValue));
}

/**
 * @threadsafety Always.
 */
Value DynamicObject::InternalGetAttribute(const String& name) const
{
	ObjectLock olock(this);

	DynamicObject::AttributeConstIterator it;
	it = m_Attributes.find(name);

	if (it == m_Attributes.end())
		return Empty;

	return it->second.GetValue();
}

/**
 * @threadsafety Always.
 */
bool DynamicObject::HasAttribute(const String& name) const
{
	ObjectLock olock(this);

	return (m_Attributes.find(name) != m_Attributes.end());
}

void DynamicObject::ClearAttributesByType(AttributeType type)
{
	assert(OwnsLock());

	DynamicObject::AttributeIterator at;
	for (at = m_Attributes.begin(); at != m_Attributes.end(); at++) {
		if (at->second.GetType() != type)
			continue;

		at->second.SetValue(0, Empty);
	}
}

/**
 * @threadsafety Always.
 */
DynamicType::Ptr DynamicObject::GetType(void) const
{
	ObjectLock olock(this);

	return DynamicType::GetByName(m_Type);
}

/**
 * @threadsafety Always.
 */
String DynamicObject::GetName(void) const
{
	ObjectLock olock(this);

	return m_Name;
}

/**
 * @threadsafety Always.
 */
bool DynamicObject::IsLocal(void) const
{
	ObjectLock olock(this);

	return m_Local;
}

/**
 * @threadsafety Always.
 */
bool DynamicObject::IsAbstract(void) const
{
	ObjectLock olock(this);

	return m_Abstract;
}

/**
 * @threadsafety Always.
 */
bool DynamicObject::IsRegistered(void) const
{
	ObjectLock olock(this);

	return m_Registered;
}

/**
 * @threadsafety Always.
 */
void DynamicObject::SetSource(const String& value)
{
	ObjectLock olock(this);

	m_Source = value;
	Touch("__source");
}

/**
 * @threadsafety Always.
 */
String DynamicObject::GetSource(void) const
{
	ObjectLock olock(this);

	return m_Source;
}

void DynamicObject::Register(void)
{
	assert(OwnsLock());

	/* It's now safe to send us attribute events. */
	SetEventSafe(true);

	/* Add this new object to the list of modified objects.
	 * We're doing this here because we can't construct
	 * a while WeakPtr from within the object's constructor. */
	{
		boost::mutex::scoped_lock lock(m_TransactionMutex);
		m_ModifiedObjects.insert(GetSelf());
	}

	{
		DynamicType::Ptr dtype = GetType();
		ObjectLock olock(dtype);

		DynamicObject::Ptr dobj = dtype->GetObject(GetName());

		DynamicObject::Ptr self = GetSelf();
		assert(!dobj || dobj == self);

		if (!dobj)
			dtype->RegisterObject(self);
	}
}

void DynamicObject::OnRegistrationCompleted(void)
{
	DynamicObject::Ptr object;

	{
		ObjectLock olock(this);
		m_Registered = true;

		Start();

		Flush();

		object = GetSelf();
	}

	OnRegistered(object);
}

void DynamicObject::Start(void)
{
	assert(OwnsLock());

	/* Nothing to do here. */
}

void DynamicObject::Unregister(void)
{
	assert(OwnsLock());

	DynamicType::Ptr dtype = GetType();
	ObjectLock olock(dtype);

	if (!dtype || !dtype->GetObject(GetName()))
		return;

	dtype->UnregisterObject(GetSelf());

	OnUnregistered(GetSelf());
}

ScriptTask::Ptr DynamicObject::MakeMethodTask(const String& method,
    const vector<Value>& arguments)
{
	assert(OwnsLock());

	Dictionary::Ptr methods = m_Methods;

	String funcName = methods->Get(method);

	if (funcName.IsEmpty())
		return ScriptTask::Ptr();

	ScriptFunction::Ptr func = ScriptFunction::GetByName(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(invalid_argument("Function '" + funcName + "' does not exist."));

	return boost::make_shared<ScriptTask>(func, arguments);
}

/*
 * @threadsafety Always.
 */
void DynamicObject::DumpObjects(const String& filename)
{
	Logger::Write(LogInformation, "base", "Dumping program state to file '" + filename + "'");

	String tempFilename = filename + ".tmp";

	fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(runtime_error("Could not open '" + filename + "' file"));

	StdioStream::Ptr sfp = boost::make_shared<StdioStream>(&fp, false);
	sfp->Start();

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		String type_name;

		{
			ObjectLock olock(type);
			type_name = type->GetName();
		}

		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			ObjectLock olock(object);

			if (object->IsLocal())
				continue;

			Dictionary::Ptr persistentObject = boost::make_shared<Dictionary>();

			persistentObject->Set("type", type_name);
			persistentObject->Set("name", object->GetName());

			int types = Attribute_Local | Attribute_Replicated;

			/* only persist properties for replicated objects or for objects
			 * that are marked as persistent */
			if (!object->GetSource().IsEmpty() /*|| object->IsPersistent()*/)
				types |= Attribute_Config;

			Dictionary::Ptr update = object->BuildUpdate(0, types);

			if (!update)
				continue;

			persistentObject->Set("update", update);

			Value value = persistentObject;
			String json = value.Serialize();

			NetString::WriteStringToStream(sfp, json);
		}
	}

	sfp->Close();

	fp.close();

#ifdef _WIN32
	_unlink(filename.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), filename.CStr()) < 0)
		BOOST_THROW_EXCEPTION(PosixException("rename() failed", errno));
}

/*
 * @threadsafety Always.
 */
void DynamicObject::RestoreObjects(const String& filename)
{
	Logger::Write(LogInformation, "base", "Restoring program state from file '" + filename + "'");

	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = boost::make_shared<StdioStream>(&fp, false);
	sfp->Start();

	unsigned long restored = 0;

	String message;
	while (NetString::ReadStringFromStream(sfp, &message)) {
		Dictionary::Ptr persistentObject = Value::Deserialize(message);

		assert(persistentObject->IsSealed());

		String type = persistentObject->Get("type");
		String name = persistentObject->Get("name");
		Dictionary::Ptr update = persistentObject->Get("update");

		bool hasConfig = update->Contains("configTx");

		DynamicType::Ptr dt = DynamicType::GetByName(type);

		if (!dt)
			BOOST_THROW_EXCEPTION(invalid_argument("Invalid type: " + type));

		DynamicObject::Ptr object = dt->GetObject(name);

		if (hasConfig && !object) {
			object = DynamicType::CreateObject(dt, update);
			ObjectLock olock(object);
			object->Register();
		} else if (object) {
			ObjectLock olock(object);
			object->ApplyUpdate(update, Attribute_All);
		}

		restored++;
	}

	sfp->Close();

	stringstream msgbuf;
	msgbuf << "Restored " << restored << " objects";
	Logger::Write(LogInformation, "base", msgbuf.str());
}

void DynamicObject::DeactivateObjects(void)
{
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			ObjectLock olock(object);
			object->Unregister();
		}
	}
}

/*
 * @threadsafety Always.
 */
double DynamicObject::GetCurrentTx(void)
{
	boost::mutex::scoped_lock lock(m_TransactionMutex);

	if (m_CurrentTx == 0) {
		/* Set the initial transaction ID. */
		m_CurrentTx = Utility::GetTime();
	}

	return m_CurrentTx;
}

void DynamicObject::Flush(void)
{
	OnFlushObject(GetCurrentTx(), GetSelf());
}

/*
 * @threadsafety Always. Caller must not hold any Object locks.
 */
void DynamicObject::NewTx(void)
{
	double tx;
	set<DynamicObject::WeakPtr> objects;

	{
		boost::mutex::scoped_lock lock(m_TransactionMutex);

		tx = m_CurrentTx;
		m_ModifiedObjects.swap(objects);
		m_CurrentTx = Utility::GetTime();
	}

	BOOST_FOREACH(const DynamicObject::WeakPtr& wobject, objects) {
		DynamicObject::Ptr object = wobject.lock();

		if (!object)
			continue;

		map<String, Value, string_iless> attrs;
		bool event_safe;

		{
			ObjectLock olock(object);
			attrs.swap(object->m_ModifiedAttributes);
			event_safe = object->GetEventSafe();
		}

		/* Send attribute events if it's safe to do so. */
		if (event_safe) {
			map<String, Value, string_iless>::iterator it;
			for (it = attrs.begin(); it != attrs.end(); it++)
				object->OnAttributeChanged(it->first, it->second);
		}
	}

	OnTransactionClosing(tx, objects);
}

void DynamicObject::OnAttributeChanged(const String&, const Value&)
{ }

/*
 * @threadsafety Always.
 */
DynamicObject::Ptr DynamicObject::GetObject(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	return dtype->GetObject(name);
}

const DynamicObject::AttributeMap& DynamicObject::GetAttributes(void) const
{
	assert(OwnsLock());

	return m_Attributes;
}

void DynamicObject::SetEventSafe(bool safe)
{
	assert(OwnsLock());

	m_EventSafe = safe;
}

bool DynamicObject::GetEventSafe(void) const
{
	assert(OwnsLock());

	return m_EventSafe;
}
