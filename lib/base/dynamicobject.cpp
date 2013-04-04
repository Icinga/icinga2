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

#include "base/dynamicobject.h"
#include "base/dynamictype.h"
#include "base/netstring.h"
#include "base/registry.h"
#include "base/stdiostream.h"
#include "base/utility.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/timer.h"
#include "base/scriptfunction.h"
#include <fstream>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

static double l_CurrentTx = 0;
static std::set<DynamicObject::WeakPtr> l_ModifiedObjects;
static boost::mutex l_TransactionMutex;
static boost::once_flag l_TransactionOnce = BOOST_ONCE_INIT;
static Timer::Ptr l_TransactionTimer;

boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnRegistered;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnUnregistered;
boost::signals2::signal<void (double, const std::set<DynamicObject::WeakPtr>&)> DynamicObject::OnTransactionClosing;
boost::signals2::signal<void (double, const DynamicObject::Ptr&)> DynamicObject::OnFlushObject;

DynamicObject::DynamicObject(const Dictionary::Ptr& serializedObject)
	: m_ConfigTx(0), m_Registered(false)
{
	RegisterAttribute("__name", Attribute_Config, &m_Name);
	RegisterAttribute("__type", Attribute_Config, &m_Type);
	RegisterAttribute("__local", Attribute_Config, &m_Local);
	RegisterAttribute("__source", Attribute_Local, &m_Source);
	RegisterAttribute("methods", Attribute_Config, &m_Methods);

	if (!serializedObject->Contains("configTx"))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Serialized object must contain a config snapshot."));

	/* apply config state from the config item/remote update;
	 * The DynamicType::CreateObject function takes care of restoring
	 * non-config state after the object has been fully constructed */
	ApplyUpdate(serializedObject, Attribute_Config);

	boost::call_once(l_TransactionOnce, &DynamicObject::Initialize);
}

DynamicObject::~DynamicObject(void)
{ }

void DynamicObject::Initialize(void)
{
	/* Set up a timer to periodically create a new transaction. */
	l_TransactionTimer = boost::make_shared<Timer>();
	l_TransactionTimer->SetInterval(0.5);
	l_TransactionTimer->OnTimerExpired.connect(boost::bind(&DynamicObject::NewTx));
	l_TransactionTimer->Start();
}

Dictionary::Ptr DynamicObject::BuildUpdate(double sinceTx, int attributeTypes) const
{
	ObjectLock olock(this);

	DynamicObject::AttributeConstIterator it;

	Dictionary::Ptr attrs = boost::make_shared<Dictionary>();

	{
		boost::mutex::scoped_lock lock(m_AttributeMutex);

		for (it = m_Attributes.begin(); it != m_Attributes.end(); ++it) {
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
	ObjectLock olock(this);

	ASSERT(serializedUpdate->IsSealed());

	Value configTxValue = serializedUpdate->Get("configTx");

	boost::mutex::scoped_lock lock(m_AttributeMutex);

	if ((allowedTypes & Attribute_Config) != 0 && !configTxValue.IsEmpty()) {
		double configTx = configTxValue;

		if (configTx > m_ConfigTx) {
			DynamicObject::AttributeIterator at;
			for (at = m_Attributes.begin(); at != m_Attributes.end(); ++at) {
				if ((at->second.GetType() & Attribute_Config) == 0)
					continue;

				at->second.SetValue(0, Empty);
			}
		}
	}

	Dictionary::Ptr attrs = serializedUpdate->Get("attrs");

	ASSERT(attrs->IsSealed());

	{
		ObjectLock alock(attrs);

		Dictionary::Iterator it;
		for (it = attrs->Begin(); it != attrs->End(); ++it) {
			if (!it->second.IsObjectType<Dictionary>())
				continue;

			Dictionary::Ptr attr = it->second;

			ASSERT(attr->IsSealed());

			int type = attr->Get("type");

			if ((type & ~allowedTypes) != 0)
				continue;

			Value data = attr->Get("data");
			double tx = attr->Get("tx");

			if (type & Attribute_Config)
				InternalRegisterAttribute(it->first, Attribute_Config);

			if (m_Attributes.find(it->first) == m_Attributes.end())
				InternalRegisterAttribute(it->first, static_cast<AttributeType>(type));

			InternalSetAttribute(it->first, data, tx, true);
		}
	}
}

void DynamicObject::RegisterAttribute(const String& name,
    AttributeType type, AttributeBase *boundAttribute)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	boost::mutex::scoped_lock lock(m_AttributeMutex);

	InternalRegisterAttribute(name, type, boundAttribute);
}

/**
 * Note: Caller must hold m_AttributeMutex.
 */
void DynamicObject::InternalRegisterAttribute(const String& name,
    AttributeType type, AttributeBase *boundAttribute)
{
	ASSERT(OwnsLock());

	AttributeHolder attr(type, boundAttribute);

	std::pair<DynamicObject::AttributeIterator, bool> tt;
	tt = m_Attributes.insert(std::make_pair(name, attr));

	if (!tt.second) {
		tt.first->second.SetType(type);

		if (boundAttribute)
			tt.first->second.Bind(boundAttribute);
	}
}

void DynamicObject::Set(const String& name, const Value& data)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	boost::mutex::scoped_lock lock(m_AttributeMutex);

	InternalSetAttribute(name, data, GetCurrentTx());
}

void DynamicObject::Touch(const String& name)
{
	ASSERT(OwnsLock());

	boost::mutex::scoped_lock lock(m_AttributeMutex);

	AttributeIterator it = m_Attributes.find(name);

	if (it == m_Attributes.end())
		BOOST_THROW_EXCEPTION(std::runtime_error("Touch() called for unknown attribute: " + name));

	it->second.SetTx(GetCurrentTx());

	m_ModifiedAttributes.insert(name);

	{
		boost::mutex::scoped_lock lock(l_TransactionMutex);
		l_ModifiedObjects.insert(GetSelf());
	}
}

Value DynamicObject::Get(const String& name) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	boost::mutex::scoped_lock lock(m_AttributeMutex);

	return InternalGetAttribute(name);
}

/**
 * Note: Caller must hold m_AttributeMutex.
 */
void DynamicObject::InternalSetAttribute(const String& name, const Value& data,
    double tx, bool allowEditConfig)
{
	ASSERT(OwnsLock());

	DynamicObject::AttributeIterator it;
	it = m_Attributes.find(name);

	if (it == m_Attributes.end()) {
		AttributeHolder attr(Attribute_Transient);
		attr.SetValue(tx, data);

		m_Attributes.insert(std::make_pair(name, attr));
	} else {
		if (!allowEditConfig && (it->second.GetType() & Attribute_Config))
			BOOST_THROW_EXCEPTION(std::runtime_error("Config properties are immutable: '" + name + "'."));

		it->second.SetValue(tx, data);

		if (it->second.GetType() & Attribute_Config)
			m_ConfigTx = tx;
	}

	if (IsRegistered()) {
		/* We can't call GetSelf() in the constructor or destructor.
		 * The Register() function will take care of adding this
		 * object to the list of modified objects later on if we can't
		 * do it here. */

		{
			boost::mutex::scoped_lock lock(l_TransactionMutex);
			l_ModifiedObjects.insert(GetSelf());
		}
	}

	m_ModifiedAttributes.insert(name);
}

/**
 * Note: Caller must hold m_AttributeMutex.
 */
Value DynamicObject::InternalGetAttribute(const String& name) const
{
	ASSERT(OwnsLock());

	DynamicObject::AttributeConstIterator it;
	it = m_Attributes.find(name);

	if (it == m_Attributes.end())
		return Empty;

	return it->second.GetValue();
}

DynamicType::Ptr DynamicObject::GetType(void) const
{
	return DynamicType::GetByName(m_Type);
}

String DynamicObject::GetName(void) const
{
	return m_Name;
}

bool DynamicObject::IsLocal(void) const
{
	return m_Local;
}

bool DynamicObject::IsRegistered(void) const
{
	ObjectLock olock(GetType());
	return m_Registered;
}

void DynamicObject::SetSource(const String& value)
{
	m_Source = value;
	Touch("__source");
}

String DynamicObject::GetSource(void) const
{
	return m_Source;
}

void DynamicObject::Register(void)
{
	ASSERT(!OwnsLock());

	/* Add this new object to the list of modified objects.
	 * We're doing this here because we can't construct
	 * a while WeakPtr from within the object's constructor. */
	{
		boost::mutex::scoped_lock lock(l_TransactionMutex);
		l_ModifiedObjects.insert(GetSelf());
	}

	DynamicType::Ptr dtype = GetType();
	dtype->RegisterObject(GetSelf());
}

void DynamicObject::OnRegistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	Start();

	OnRegistered(GetSelf());
}

void DynamicObject::OnUnregistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	Stop();

	OnUnregistered(GetSelf());
}

void DynamicObject::Start(void)
{
	ASSERT(!OwnsLock());

	/* Nothing to do here. */
}

void DynamicObject::Stop(void)
{
	ASSERT(!OwnsLock());

	/* Nothing to do here. */
}

void DynamicObject::Unregister(void)
{
	ASSERT(!OwnsLock());

	DynamicType::Ptr dtype = GetType();

	if (!dtype)
		return;

	dtype->UnregisterObject(GetSelf());
}

Value DynamicObject::InvokeMethod(const String& method,
    const std::vector<Value>& arguments)
{
	Dictionary::Ptr methods;

	methods = m_Methods;

	if (!methods)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Method '" + method + "' does not exist."));

	String funcName = methods->Get(method);

	if (funcName.IsEmpty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Method '" + method + "' does not exist."));

	ScriptFunction::Ptr func = ScriptFunctionRegistry::GetInstance()->GetItem(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Function '" + funcName + "' does not exist."));

	return func->Invoke(arguments);
}

void DynamicObject::DumpObjects(const String& filename)
{
	Log(LogInformation, "base", "Dumping program state to file '" + filename + "'");

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + filename + "' file"));

	StdioStream::Ptr sfp = boost::make_shared<StdioStream>(&fp, false);

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			if (object->IsLocal())
				continue;

			Dictionary::Ptr persistentObject = boost::make_shared<Dictionary>();

			persistentObject->Set("type", type->GetName());
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

	if (rename(tempFilename.CStr(), filename.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}
}

void DynamicObject::RestoreObjects(const String& filename)
{
	Log(LogInformation, "base", "Restoring program state from file '" + filename + "'");

	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = boost::make_shared<StdioStream>(&fp, false);

	unsigned long restored = 0;

	String message;
	while (NetString::ReadStringFromStream(sfp, &message)) {
		Dictionary::Ptr persistentObject = Value::Deserialize(message);

		ASSERT(persistentObject->IsSealed());

		String type = persistentObject->Get("type");
		String name = persistentObject->Get("name");
		Dictionary::Ptr update = persistentObject->Get("update");

		bool hasConfig = update->Contains("configTx");

		DynamicType::Ptr dt = DynamicType::GetByName(type);

		if (!dt)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type: " + type));

		DynamicObject::Ptr object = dt->GetObject(name);

		if (hasConfig && !object) {
			object = dt->DynamicType::CreateObject(update);
			object->Register();
		} else if (object) {
			object->ApplyUpdate(update, Attribute_All);
		}

		restored++;
	}

	sfp->Close();

	std::ostringstream msgbuf;
	msgbuf << "Restored " << restored << " objects";
	Log(LogInformation, "base", msgbuf.str());
}

void DynamicObject::DeactivateObjects(void)
{
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			object->Unregister();
		}
	}
}

double DynamicObject::GetCurrentTx(void)
{
	boost::mutex::scoped_lock lock(l_TransactionMutex);

	if (l_CurrentTx == 0) {
		/* Set the initial transaction ID. */
		l_CurrentTx = Utility::GetTime();
	}

	return l_CurrentTx;
}

void DynamicObject::Flush(void)
{
	OnFlushObject(GetCurrentTx(), GetSelf());
}

void DynamicObject::NewTx(void)
{
	double tx;
	std::set<DynamicObject::WeakPtr> objects;

	{
		boost::mutex::scoped_lock lock(l_TransactionMutex);

		tx = l_CurrentTx;
		l_ModifiedObjects.swap(objects);
		l_CurrentTx = Utility::GetTime();
	}

	BOOST_FOREACH(const DynamicObject::WeakPtr& wobject, objects) {
		DynamicObject::Ptr object = wobject.lock();

		if (!object || !object->IsRegistered())
			continue;

		std::set<String, string_iless> attrs;

		{
			ObjectLock olock(object);
			attrs.swap(object->m_ModifiedAttributes);
		}

		BOOST_FOREACH(const String& attr, attrs) {
			object->OnAttributeChanged(attr);
		}
	}

	OnTransactionClosing(tx, objects);
}

void DynamicObject::OnAttributeChanged(const String&)
{
	ASSERT(!OwnsLock());
}

DynamicObject::Ptr DynamicObject::GetObject(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	return dtype->GetObject(name);
}

const DynamicObject::AttributeMap& DynamicObject::GetAttributes(void) const
{
	ASSERT(OwnsLock());

	return m_Attributes;
}
