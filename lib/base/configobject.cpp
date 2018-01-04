/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/configobject.hpp"
#include "base/configobject.tcpp"
#include "base/configtype.hpp"
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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(ConfigObject, ConfigObject::GetPrototype());

boost::signals2::signal<void (const ConfigObject::Ptr&)> ConfigObject::OnStateChanged;

ConfigObject::ConfigObject()
{ }

bool ConfigObject::IsActive() const
{
	return GetActive();
}

bool ConfigObject::IsPaused() const
{
	return GetPaused();
}

void ConfigObject::SetExtension(const String& key, const Value& value)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions) {
		extensions = new Dictionary();
		SetExtensions(extensions);
	}

	extensions->Set(key, value);
}

Value ConfigObject::GetExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return Empty;

	return extensions->Get(key);
}

void ConfigObject::ClearExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return;

	extensions->Remove(key);
}

class ModAttrValidationUtils final : public ValidationUtils
{
public:
	virtual bool ValidateName(const String& type, const String& name) const override
	{
		Type::Ptr ptype = Type::GetByName(type);
		ConfigType *dtype = dynamic_cast<ConfigType *>(ptype.get());

		if (!dtype)
			return false;

		if (!dtype->GetObject(name))
			return false;

		return true;
	}
};

void ConfigObject::ModifyAttribute(const String& attr, const Value& value, bool updateVersion)
{
	Dictionary::Ptr original_attributes = GetOriginalAttributes();
	bool updated_original_attributes = false;

	Type::Ptr type = GetReflectionType();

	std::vector<String> tokens;
	boost::algorithm::split(tokens, attr, boost::is_any_of("."));

	String fieldName = tokens[0];

	int fid = type->GetFieldId(fieldName);
	Field field = type->GetFieldInfo(fid);

	if (field.Attributes & FANoUserModify)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Attribute cannot be modified."));

	if (field.Attributes & FAConfig) {
		if (!original_attributes) {
			original_attributes = new Dictionary();
			SetOriginalAttributes(original_attributes, true);
		}
	}

	Value oldValue = GetField(fid);
	Value newValue;

	if (tokens.size() > 1) {
		newValue = oldValue.Clone();
		Value current = newValue;

		if (current.IsEmpty()) {
			current = new Dictionary();
			newValue = current;
		}

		String prefix = tokens[0];

		for (std::vector<String>::size_type i = 1; i < tokens.size() - 1; i++) {
			if (!current.IsObjectType<Dictionary>())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

			Dictionary::Ptr dict = current;

			const String& key = tokens[i];
			prefix += "." + key;

			if (!dict->Get(key, &current)) {
				current = new Dictionary();
				dict->Set(key, current);
			}
		}

		if (!current.IsObjectType<Dictionary>())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

		Dictionary::Ptr dict = current;

		const String& key = tokens[tokens.size() - 1];
		prefix += "." + key;

		/* clone it for original attributes */
		oldValue = dict->Get(key).Clone();

		if (field.Attributes & FAConfig) {
			updated_original_attributes = true;

			if (oldValue.IsObjectType<Dictionary>()) {
				Dictionary::Ptr oldDict = oldValue;
				ObjectLock olock(oldDict);
				for (const auto& kv : oldDict) {
					String key = prefix + "." + kv.first;
					if (!original_attributes->Contains(key))
						original_attributes->Set(key, kv.second);
				}

				/* store the new value as null */
				if (value.IsObjectType<Dictionary>()) {
					Dictionary::Ptr valueDict = value;
					ObjectLock olock(valueDict);
					for (const auto& kv : valueDict) {
						String key = attr + "." + kv.first;
						if (!original_attributes->Contains(key))
							original_attributes->Set(key, Empty);
					}
				}
			} else if (!original_attributes->Contains(attr))
				original_attributes->Set(attr, oldValue);
		}

		dict->Set(key, value);
	} else {
		newValue = value;

		if (field.Attributes & FAConfig) {
			if (!original_attributes->Contains(attr)) {
				updated_original_attributes = true;
				original_attributes->Set(attr, oldValue);
			}
		}
	}

	ModAttrValidationUtils utils;
	ValidateField(fid, newValue, utils);

	SetField(fid, newValue);

	if (updateVersion && (field.Attributes & FAConfig))
		SetVersion(Utility::GetTime());

	if (updated_original_attributes)
		NotifyOriginalAttributes();
}

void ConfigObject::RestoreAttribute(const String& attr, bool updateVersion)
{
	Type::Ptr type = GetReflectionType();

	std::vector<String> tokens;
	boost::algorithm::split(tokens, attr, boost::is_any_of("."));

	String fieldName = tokens[0];

	int fid = type->GetFieldId(fieldName);

	Value currentValue = GetField(fid);

	Dictionary::Ptr original_attributes = GetOriginalAttributes();

	if (!original_attributes)
		return;

	Value oldValue = original_attributes->Get(attr);
	Value newValue;

	if (tokens.size() > 1) {
		newValue = currentValue.Clone();
		Value current = newValue;

		if (current.IsEmpty())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot restore non-existent object attribute"));

		String prefix = tokens[0];

		for (std::vector<String>::size_type i = 1; i < tokens.size() - 1; i++) {
			if (!current.IsObjectType<Dictionary>())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

			Dictionary::Ptr dict = current;

			const String& key = tokens[i];
			prefix += "." + key;

			if (!dict->Contains(key))
				BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot restore non-existent object attribute"));

			current = dict->Get(key);
		}

		if (!current.IsObjectType<Dictionary>())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

		Dictionary::Ptr dict = current;

		const String& key = tokens[tokens.size() - 1];
		prefix += "." + key;

		std::vector<String> restoredAttrs;

		{
			ObjectLock olock(original_attributes);
			for (const auto& kv : original_attributes) {
				std::vector<String> originalTokens;
				boost::algorithm::split(originalTokens, kv.first, boost::is_any_of("."));

				if (tokens.size() > originalTokens.size())
					continue;

				bool match = true;
				for (std::vector<String>::size_type i = 0; i < tokens.size(); i++) {
					if (tokens[i] != originalTokens[i]) {
						match = false;
						break;
					}
				}

				if (!match)
					continue;

				Dictionary::Ptr dict;

				if (tokens.size() == originalTokens.size())
					dict = current;
				else {
					Value currentSub = current;

					for (std::vector<String>::size_type i = tokens.size() - 1; i < originalTokens.size() - 1; i++) {
						dict = currentSub;
						currentSub = dict->Get(originalTokens[i]);

						if (!currentSub.IsObjectType<Dictionary>()) {
							currentSub = new Dictionary();
							dict->Set(originalTokens[i], currentSub);
						}
					}

					dict = currentSub;
				}

				dict->Set(originalTokens[originalTokens.size() - 1], kv.second);
				restoredAttrs.push_back(kv.first);
			}
		}

		for (const String& attr : restoredAttrs)
			original_attributes->Remove(attr);


	} else {
		newValue = oldValue;
	}

	original_attributes->Remove(attr);
	SetField(fid, newValue);

	if (updateVersion)
		SetVersion(Utility::GetTime());
}

bool ConfigObject::IsAttributeModified(const String& attr) const
{
	Dictionary::Ptr original_attributes = GetOriginalAttributes();

	if (!original_attributes)
		return false;

	return original_attributes->Contains(attr);
}

void ConfigObject::Register()
{
	ASSERT(!OwnsLock());

	TypeImpl<ConfigObject>::Ptr type = static_pointer_cast<TypeImpl<ConfigObject> >(GetReflectionType());
	type->RegisterObject(this);
}

void ConfigObject::Unregister()
{
	ASSERT(!OwnsLock());

	TypeImpl<ConfigObject>::Ptr type = static_pointer_cast<TypeImpl<ConfigObject> >(GetReflectionType());
	type->UnregisterObject(this);
}

void ConfigObject::Start(bool runtimeCreated)
{
	ObjectImpl<ConfigObject>::Start(runtimeCreated);

	ObjectLock olock(this);

	SetStartCalled(true);
}

void ConfigObject::PreActivate()
{
	CONTEXT("Setting 'active' to true for object '" + GetName() + "' of type '" + GetReflectionType()->GetName() + "'");

	ASSERT(!IsActive());
	SetActive(true, true);
}

void ConfigObject::Activate(bool runtimeCreated)
{
	CONTEXT("Activating object '" + GetName() + "' of type '" + GetReflectionType()->GetName() + "'");

	{
		ObjectLock olock(this);

		Start(runtimeCreated);

		ASSERT(GetStartCalled());

		if (GetHAMode() == HARunEverywhere)
			SetAuthority(true);
	}

	NotifyActive();
}

void ConfigObject::Stop(bool runtimeRemoved)
{
	ObjectImpl<ConfigObject>::Stop(runtimeRemoved);

	ObjectLock olock(this);

	SetStopCalled(true);
}

void ConfigObject::Deactivate(bool runtimeRemoved)
{
	CONTEXT("Deactivating object '" + GetName() + "' of type '" + GetReflectionType()->GetName() + "'");

	{
		ObjectLock olock(this);

		if (!IsActive())
			return;

		SetActive(false, true);

		SetAuthority(false);

		Stop(runtimeRemoved);
	}

	ASSERT(GetStopCalled());

	NotifyActive();
}

void ConfigObject::OnConfigLoaded()
{
	/* Nothing to do here. */
}

void ConfigObject::OnAllConfigLoaded()
{
	static ConfigType *ctype;

	if (!ctype) {
		Type::Ptr type = Type::GetByName("Zone");
		ctype = dynamic_cast<ConfigType *>(type.get());
	}

	String zoneName = GetZoneName();

	if (!zoneName.IsEmpty())
		m_Zone = ctype->GetObject(zoneName);
}

void ConfigObject::CreateChildObjects(const Type::Ptr& childType)
{
	/* Nothing to do here. */
}

void ConfigObject::OnStateLoaded()
{
	/* Nothing to do here. */
}

void ConfigObject::Pause()
{
	SetPauseCalled(true);
}

void ConfigObject::Resume()
{
	SetResumeCalled(true);
}

void ConfigObject::SetAuthority(bool authority)
{
	ObjectLock olock(this);

	if (authority && GetPaused()) {
		SetResumeCalled(false);
		Resume();
		ASSERT(GetResumeCalled());
		SetPaused(false);
	} else if (!authority && !GetPaused()) {
		SetPaused(true);
		SetPauseCalled(false);
		Pause();
		ASSERT(GetPauseCalled());
	}
}

void ConfigObject::DumpObjects(const String& filename, int attributeTypes)
{
	Log(LogInformation, "ConfigObject")
		<< "Dumping program state to file '" << filename << "'";

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(filename + ".XXXXXX", 0600, fp);
	fp.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
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

void ConfigObject::RestoreObject(const String& message, int attributeTypes)
{
	Dictionary::Ptr persistentObject = JsonDecode(message);

	String type = persistentObject->Get("type");
	String name = persistentObject->Get("name");

	ConfigObject::Ptr object = GetObject(type, name);

	if (!object)
		return;

#ifdef I2_DEBUG
	Log(LogDebug, "ConfigObject")
		<< "Restoring object '" << name << "' of type '" << type << "'.";
#endif /* I2_DEBUG */
	Dictionary::Ptr update = persistentObject->Get("update");
	Deserialize(object, update, false, attributeTypes);
	object->OnStateLoaded();
	object->SetStateLoaded(true);
}

void ConfigObject::RestoreObjects(const String& filename, int attributeTypes)
{
	if (!Utility::PathExists(filename))
		return;

	Log(LogInformation, "ConfigObject")
		<< "Restoring program state from file '" << filename << "'";

	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream (&fp, false);

	unsigned long restored = 0;

	WorkQueue upq(25000, Application::GetConcurrency());
	upq.SetName("ConfigObject::RestoreObjects");

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		upq.Enqueue(std::bind(&ConfigObject::RestoreObject, message, attributeTypes));
		restored++;
	}

	sfp->Close();

	upq.Join();

	unsigned long no_state = 0;

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			if (!object->GetStateLoaded()) {
				object->OnStateLoaded();
				object->SetStateLoaded(true);

				no_state++;
			}
		}
	}

	Log(LogInformation, "ConfigObject")
		<< "Restored " << restored << " objects. Loaded " << no_state << " new objects without state.";
}

void ConfigObject::StopObjects()
{
	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			object->Deactivate();
		}
	}
}

void ConfigObject::DumpModifiedAttributes(const std::function<void(const ConfigObject::Ptr&, const String&, const Value&)>& callback)
{
	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			Dictionary::Ptr originalAttributes = object->GetOriginalAttributes();

			if (!originalAttributes)
				continue;

			ObjectLock olock(originalAttributes);
			for (const Dictionary::Pair& kv : originalAttributes) {
				String key = kv.first;

				Type::Ptr type = object->GetReflectionType();

				std::vector<String> tokens;
				boost::algorithm::split(tokens, key, boost::is_any_of("."));

				String fieldName = tokens[0];
				int fid = type->GetFieldId(fieldName);

				Value currentValue = object->GetField(fid);
				Value modifiedValue;

				if (tokens.size() > 1) {
					Value current = currentValue;

					for (std::vector<String>::size_type i = 1; i < tokens.size() - 1; i++) {
						if (!current.IsObjectType<Dictionary>())
							BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

						Dictionary::Ptr dict = current;
						const String& key = tokens[i];

						if (!dict->Contains(key))
							break;

						current = dict->Get(key);
					}

					if (!current.IsObjectType<Dictionary>())
						BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

					Dictionary::Ptr dict = current;
					const String& key = tokens[tokens.size() - 1];

					modifiedValue = dict->Get(key);
				} else
					modifiedValue = currentValue;

				callback(object, key, modifiedValue);
			}
		}
	}

}

ConfigObject::Ptr ConfigObject::GetObject(const String& type, const String& name)
{
	Type::Ptr ptype = Type::GetByName(type);
	ConfigType *ctype = dynamic_cast<ConfigType *>(ptype.get());

	if (!ctype)
		return nullptr;

	return ctype->GetObject(name);
}

ConfigObject::Ptr ConfigObject::GetZone() const
{
	return m_Zone;
}

Dictionary::Ptr ConfigObject::GetSourceLocation() const
{
	DebugInfo di = GetDebugInfo();

	Dictionary::Ptr result = new Dictionary();
	result->Set("path", di.Path);
	result->Set("first_line", di.FirstLine);
	result->Set("first_column", di.FirstColumn);
	result->Set("last_line", di.LastLine);
	result->Set("last_column", di.LastColumn);
	return result;
}

NameComposer::~NameComposer()
{ }
