/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/dynamicobject.hpp"
#include "base/dynamicobject.tcpp"
#include "base/dynamictype.hpp"
#include "base/serializer.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/function.hpp"
#include "base/initialize.hpp"
#include "base/workqueue.hpp"
#include "base/context.hpp"
#include "base/application.hpp"
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(DynamicObject, DynamicObject::GetPrototype());

boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStateChanged;

DynamicObject::DynamicObject(void)
{ }

DynamicType::Ptr DynamicObject::GetType(void) const
{
	return DynamicType::GetByName(GetTypeNameV());
}

bool DynamicObject::IsActive(void) const
{
	return GetActive();
}

bool DynamicObject::IsPaused(void) const
{
	return GetPaused();
}

void DynamicObject::SetExtension(const String& key, const Value& value)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions) {
		extensions = new Dictionary();
		SetExtensions(extensions);
	}

	extensions->Set(key, value);
}

Value DynamicObject::GetExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return Empty;

	return extensions->Get(key);
}

void DynamicObject::ClearExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return;

	extensions->Remove(key);
}

void DynamicObject::ModifyAttribute(const String& attr, const Value& value)
{
	Dictionary::Ptr original_attributes = GetOriginalAttributes();
	bool updated_original_attributes = false;

	Type::Ptr type = GetReflectionType();
	int fid = type->GetFieldId(attr);
	Field field = type->GetFieldInfo(fid);

	if (field.Attributes & FAConfig) {
		if (!original_attributes) {
			original_attributes = new Dictionary();
			SetOriginalAttributes(original_attributes, true);
		}

		Value attrVal = GetField(fid);

		if (!original_attributes->Contains(attr)) {
			updated_original_attributes = true;
			original_attributes->Set(attr, attrVal);
		}
	}

	//TODO: validation, vars.os
	SetField(fid, value);

	if (updated_original_attributes)
		NotifyOriginalAttributes();
}

void DynamicObject::RestoreAttribute(const String& attr)
{
	Dictionary::Ptr original_attributes = GetOriginalAttributes();

	if (!original_attributes || !original_attributes->Contains(attr))
		return;

	Value attrVal = original_attributes->Get(attr);

	SetField(GetReflectionType()->GetFieldId(attr), attrVal);
	original_attributes->Remove(attr);
}

bool DynamicObject::IsAttributeModified(const String& attr) const
{
	Dictionary::Ptr original_attributes = GetOriginalAttributes();

	if (!original_attributes)
		return false;

	return original_attributes->Contains(attr);
}

void DynamicObject::Register(void)
{
	ASSERT(!OwnsLock());

	DynamicType::Ptr dtype = GetType();
	dtype->RegisterObject(this);
}

void DynamicObject::Start(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStartCalled(true);
}

void DynamicObject::Activate(void)
{
	CONTEXT("Activating object '" + GetName() + "' of type '" + GetType()->GetName() + "'");

	ASSERT(!OwnsLock());

	Start();

	ASSERT(GetStartCalled());

	{
		ObjectLock olock(this);
		ASSERT(!IsActive());
		SetActive(true, true);
	}

	SetAuthority(true);
	
	NotifyActive();
}

void DynamicObject::Stop(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStopCalled(true);
}

void DynamicObject::Deactivate(void)
{
	CONTEXT("Deactivating object '" + GetName() + "' of type '" + GetType()->GetName() + "'");

	ASSERT(!OwnsLock());

	SetAuthority(false);

	{
		ObjectLock olock(this);

		if (!IsActive())
			return;

		SetActive(false, true);
	}

	Stop();

	ASSERT(GetStopCalled());

	NotifyActive();
}

void DynamicObject::OnConfigLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::OnAllConfigLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::CreateChildObjects(const Type::Ptr& childType)
{
	/* Nothing to do here. */
}

void DynamicObject::OnStateLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::Pause(void)
{
	SetPauseCalled(true);
}

void DynamicObject::Resume(void)
{
	SetResumeCalled(true);
}

void DynamicObject::SetAuthority(bool authority)
{
	if (authority && GetPaused()) {
		SetResumeCalled(false);
		Resume();
		ASSERT(GetResumeCalled());
		SetPaused(false);
	} else if (!authority && !GetPaused()) {
		SetPauseCalled(false);
		Pause();
		ASSERT(GetPauseCalled());
		SetPaused(true);
	}
}

void DynamicObject::DumpObjects(const String& filename, int attributeTypes)
{
	Log(LogInformation, "DynamicObject")
	    << "Dumping program state to file '" << filename << "'";

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			Dictionary::Ptr persistentObject = new Dictionary();

			persistentObject->Set("type", type->GetName());
			persistentObject->Set("name", object->GetName());

			Dictionary::Ptr update = Serialize(object, attributeTypes);

			if (!update)
				continue;

			persistentObject->Set("update", update);

			String json = JsonEncode(persistentObject);

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

void DynamicObject::RestoreObject(const String& message, int attributeTypes)
{
	Dictionary::Ptr persistentObject = JsonDecode(message);

	String type = persistentObject->Get("type");

	DynamicType::Ptr dt = DynamicType::GetByName(type);

	if (!dt)
		return;

	String name = persistentObject->Get("name");

	DynamicObject::Ptr object = dt->GetObject(name);

	if (!object)
		return;

	ASSERT(!object->IsActive());
#ifdef I2_DEBUG
	Log(LogDebug, "DynamicObject")
	    << "Restoring object '" << name << "' of type '" << type << "'.";
#endif /* I2_DEBUG */
	Dictionary::Ptr update = persistentObject->Get("update");
	Deserialize(object, update, false, attributeTypes);
	object->OnStateLoaded();
	object->SetStateLoaded(true);
}

void DynamicObject::RestoreObjects(const String& filename, int attributeTypes)
{
	if (!Utility::PathExists(filename))
		return;

	Log(LogInformation, "DynamicObject")
	    << "Restoring program state from file '" << filename << "'";

	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream (&fp, false);

	unsigned long restored = 0;

	WorkQueue upq(25000, Application::GetConcurrency());

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		upq.Enqueue(boost::bind(&DynamicObject::RestoreObject, message, attributeTypes));
		restored++;
	}

	sfp->Close();

	upq.Join();

	unsigned long no_state = 0;

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			if (!object->GetStateLoaded()) {
				object->OnStateLoaded();
				object->SetStateLoaded(true);

				no_state++;
			}
		}
	}

	Log(LogInformation, "DynamicObject")
	    << "Restored " << restored << " objects. Loaded " << no_state << " new objects without state.";
}

void DynamicObject::StopObjects(void)
{
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			object->Deactivate();
		}
	}
}

DynamicObject::Ptr DynamicObject::GetObject(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	return dtype->GetObject(name);
}
