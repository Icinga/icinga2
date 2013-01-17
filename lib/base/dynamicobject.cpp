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
set<DynamicObject::Ptr> DynamicObject::m_ModifiedObjects;

boost::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnRegistered;
boost::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnUnregistered;
boost::signal<void (const set<DynamicObject::Ptr>&)> DynamicObject::OnTransactionClosing;

DynamicObject::DynamicObject(const Dictionary::Ptr& serializedObject)
	: m_ConfigTx(0)
{
	RegisterAttribute("__name", Attribute_Config);
	RegisterAttribute("__type", Attribute_Config);
	RegisterAttribute("__local", Attribute_Config);
	RegisterAttribute("__abstract", Attribute_Config);
	RegisterAttribute("__source", Attribute_Local);
	RegisterAttribute("methods", Attribute_Config);

	if (!serializedObject->Contains("configTx"))
		throw_exception(invalid_argument("Serialized object must contain a config snapshot."));

	/* apply config state from the config item/remote update;
	 * The DynamicObject::Create function takes care of restoring
	 * non-config state after the object has been fully constructed */
	InternalApplyUpdate(serializedObject, Attribute_Config, true);
}

Dictionary::Ptr DynamicObject::BuildUpdate(double sinceTx, int attributeTypes) const
{
	DynamicObject::AttributeConstIterator it;

	Dictionary::Ptr attrs = boost::make_shared<Dictionary>();

	for (it = m_Attributes.begin(); it != m_Attributes.end(); it++) {
		if (it->second.Type == Attribute_Transient)
			continue;

		if ((it->second.Type & attributeTypes) == 0)
			continue;

		if (it->second.Tx == 0)
			continue;

		if (it->second.Tx < sinceTx && !(it->second.Type == Attribute_Config && m_ConfigTx >= sinceTx))
			continue;

		Dictionary::Ptr attr = boost::make_shared<Dictionary>();
		attr->Set("data", it->second.Data);
		attr->Set("type", it->second.Type);
		attr->Set("tx", it->second.Tx);

		attrs->Set(it->first, attr);
	}

	Dictionary::Ptr update = boost::make_shared<Dictionary>();
	update->Set("attrs", attrs);

	if (m_ConfigTx >= sinceTx && attributeTypes & Attribute_Config)
		update->Set("configTx", m_ConfigTx);
	else if (attrs->GetLength() == 0)
		return Dictionary::Ptr();

	return update;
}

void DynamicObject::ApplyUpdate(const Dictionary::Ptr& serializedUpdate,
    int allowedTypes)
{
	InternalApplyUpdate(serializedUpdate, allowedTypes, false);
}

void DynamicObject::InternalApplyUpdate(const Dictionary::Ptr& serializedUpdate,
    int allowedTypes, bool suppressEvents)
{
	double configTx = 0;
	if ((allowedTypes & Attribute_Config) != 0 &&
	    serializedUpdate->Contains("configTx")) {
		configTx = serializedUpdate->Get("configTx");

		if (configTx > m_ConfigTx)
			ClearAttributesByType(Attribute_Config);
	}

	Dictionary::Ptr attrs = serializedUpdate->Get("attrs");

	Dictionary::Iterator it;
	for (it = attrs->Begin(); it != attrs->End(); it++) {
		if (!it->second.IsObjectType<Dictionary>())
			continue;

		Dictionary::Ptr attr = it->second;

		int type = attr->Get("type");

		if ((type & ~allowedTypes) != 0)
			continue;

		Value data = attr->Get("data");
		double tx = attr->Get("tx");

		if (type & Attribute_Config)
			RegisterAttribute(it->first, Attribute_Config);

		if (!HasAttribute(it->first))
			RegisterAttribute(it->first, static_cast<DynamicAttributeType>(type));

		InternalSetAttribute(it->first, data, tx, suppressEvents);
	}
}

void DynamicObject::RegisterAttribute(const String& name,
    DynamicAttributeType type)
{
	DynamicAttribute attr;
	attr.Type = type;
	attr.Tx = 0;

	pair<DynamicObject::AttributeIterator, bool> tt;
	tt = m_Attributes.insert(make_pair(name, attr));

	if (!tt.second)
		tt.first->second.Type = type;
}

void DynamicObject::Set(const String& name, const Value& data)
{
	InternalSetAttribute(name, data, GetCurrentTx());
}

void DynamicObject::Touch(const String& name)
{
	InternalSetAttribute(name, InternalGetAttribute(name), GetCurrentTx());
}

Value DynamicObject::Get(const String& name) const
{
	return InternalGetAttribute(name);
}

void DynamicObject::InternalSetAttribute(const String& name, const Value& data,
    double tx, bool suppressEvent)
{
	DynamicAttribute attr;
	attr.Type = Attribute_Transient;
	attr.Data = data;
	attr.Tx = tx;

	pair<DynamicObject::AttributeIterator, bool> tt;
	tt = m_Attributes.insert(make_pair(name, attr));

	Value oldValue;

	if (!tt.second && tx >= tt.first->second.Tx) {
		oldValue = tt.first->second.Data;
		tt.first->second.Data = data;
		tt.first->second.Tx = tx;
	}

	if (tt.first->second.Type & Attribute_Config)
		m_ConfigTx = tx;

	if (!suppressEvent) {
		m_ModifiedObjects.insert(GetSelf());
		OnAttributeChanged(name, oldValue);
	}
}

Value DynamicObject::InternalGetAttribute(const String& name) const
{
	DynamicObject::AttributeConstIterator it;
	it = m_Attributes.find(name);

	if (it == m_Attributes.end())
		return Empty;

	return it->second.Data;
}

bool DynamicObject::HasAttribute(const String& name) const
{
	return (m_Attributes.find(name) != m_Attributes.end());
}

void DynamicObject::ClearAttributesByType(DynamicAttributeType type)
{
	DynamicObject::AttributeIterator prev, at;
	for (at = m_Attributes.begin(); at != m_Attributes.end(); ) {
		if (at->second.Type == type) {
			prev = at;
			at++;
			m_Attributes.erase(prev);

			continue;
		}

		at++;
	}
}

DynamicType::Ptr DynamicObject::GetType(void) const
{
	String name = Get("__type");
	return DynamicType::GetByName(name);
}

String DynamicObject::GetName(void) const
{
	return Get("__name");
}

bool DynamicObject::IsLocal(void) const
{
	Value value = Get("__local");

	if (value.IsEmpty())
		return false;

	return (value != 0);
}

bool DynamicObject::IsAbstract(void) const
{
	Value value = Get("__abstract");

	if (value.IsEmpty())
		return false;

	return (value != 0);
}

void DynamicObject::SetSource(const String& value)
{
	Set("__source", value);
}

String DynamicObject::GetSource(void) const
{
	return Get("__source");
}

void DynamicObject::Register(void)
{
	assert(Application::IsMainThread());

	DynamicObject::Ptr dobj = GetType()->GetObject(GetName());
	DynamicObject::Ptr self = GetSelf();
	assert(!dobj || dobj == self);

	GetType()->RegisterObject(self);

	OnRegistered(GetSelf());

	Start();
}

void DynamicObject::Start(void)
{
	/* Nothing to do here. */
}

void DynamicObject::Unregister(void)
{
	assert(Application::IsMainThread());

	if (GetType()->GetObject(GetName()))
		return;

	GetType()->UnregisterObject(GetSelf());

	OnUnregistered(GetSelf());
}

ScriptTask::Ptr DynamicObject::InvokeMethod(const String& method,
    const vector<Value>& arguments, ScriptTask::CompletionCallback callback)
{
	Value value = Get("methods");

	if (!value.IsObjectType<Dictionary>())
		return ScriptTask::Ptr();

	Dictionary::Ptr methods = value;
	if (!methods->Contains(method))
		return ScriptTask::Ptr();

	String funcName = methods->Get(method);

	ScriptFunction::Ptr func = ScriptFunction::GetByName(funcName);

	if (!func)
		throw_exception(invalid_argument("Function '" + funcName + "' does not exist."));

	ScriptTask::Ptr task = boost::make_shared<ScriptTask>(func, arguments);
	task->Start(callback);

	return task;
}

void DynamicObject::DumpObjects(const String& filename)
{
	Logger::Write(LogInformation, "base", "Dumping program state to file '" + filename + "'");

	String tempFilename = filename + ".tmp";

	fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		throw_exception(runtime_error("Could not open '" + filename + "' file"));

	StdioStream::Ptr sfp = boost::make_shared<StdioStream>(&fp, false);
	sfp->Start();

	DynamicType::Ptr type;
	BOOST_FOREACH(tie(tuples::ignore, type), DynamicType::GetTypes()) {
		DynamicObject::Ptr object;
		BOOST_FOREACH(tie(tuples::ignore, object), type->GetObjects()) {
			if (object->IsLocal())
				continue;

			Dictionary::Ptr persistentObject = boost::make_shared<Dictionary>();

			persistentObject->Set("type", object->GetType()->GetName());
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
		throw_exception(PosixException("rename() failed", errno));
}

void DynamicObject::RestoreObjects(const String& filename)
{
	Logger::Write(LogInformation, "base", "Restoring program state from file '" + filename + "'");

	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = boost::make_shared<StdioStream>(&fp, false);
	sfp->Start();

	String message;
	while (NetString::ReadStringFromStream(sfp, &message)) {
		Dictionary::Ptr persistentObject = Value::Deserialize(message);

		String type = persistentObject->Get("type");
		String name = persistentObject->Get("name");
		Dictionary::Ptr update = persistentObject->Get("update");

		bool hasConfig = update->Contains("configTx");

		DynamicType::Ptr dt = DynamicType::GetByName(type);

		if (!dt)
			throw_exception(invalid_argument("Invalid type: " + type));

		DynamicObject::Ptr object = dt->GetObject(name);

		if (hasConfig && !object) {
			object = dt->CreateObject(update);
			object->Register();
		} else if (object) {
			object->ApplyUpdate(update, Attribute_All);
		}
	}

	sfp->Close();
}

void DynamicObject::DeactivateObjects(void)
{
	DynamicType::TypeMap::iterator tt;
	for (tt = DynamicType::GetTypes().begin(); tt != DynamicType::GetTypes().end(); tt++) {
		DynamicType::NameMap::iterator nt;

		while ((nt = tt->second->GetObjects().begin()) != tt->second->GetObjects().end()) {
			DynamicObject::Ptr object = nt->second;

			object->Unregister();
		}
	}
}

double DynamicObject::GetCurrentTx(void)
{
	assert(m_CurrentTx != 0);

	return m_CurrentTx;
}

void DynamicObject::BeginTx(void)
{
	m_CurrentTx = Utility::GetTime();
}

void DynamicObject::FinishTx(void)
{
	OnTransactionClosing(m_ModifiedObjects);
	m_ModifiedObjects.clear();

	m_CurrentTx = 0;
}

void DynamicObject::OnAttributeChanged(const String&, const Value&)
{ }

DynamicObject::Ptr DynamicObject::GetObject(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	return dtype->GetObject(name);
}
