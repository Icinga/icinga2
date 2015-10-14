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
#include "config/configitem.hpp"
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(ConfigObject, ConfigObject::GetPrototype());

boost::signals2::signal<void (const ConfigObject::Ptr&)> ConfigObject::OnStateChanged;

ConfigObject::ConfigObject(void)
{ }

ConfigType::Ptr ConfigObject::GetType(void) const
{
	return ConfigType::GetByName(GetTypeNameV());
}

bool ConfigObject::IsActive(void) const
{
	return GetActive();
}

bool ConfigObject::IsPaused(void) const
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

class ModAttrValidationUtils : public ValidationUtils
{
public:
	virtual bool ValidateName(const String& type, const String& name) const override
	{
		ConfigType::Ptr dtype = ConfigType::GetByName(type);

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

			if (!dict->Contains(key)) {
				current = new Dictionary();
				dict->Set(key, current);
			} else {
				current = dict->Get(key);
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
				BOOST_FOREACH(const Dictionary::Pair& kv, oldDict) {
					String key = prefix + "." + kv.first;
					if (!original_attributes->Contains(key))
						original_attributes->Set(key, kv.second);
				}

				/* store the new value as null */
				if (value.IsObjectType<Dictionary>()) {
					Dictionary::Ptr valueDict = value;
					ObjectLock olock(valueDict);
					BOOST_FOREACH(const Dictionary::Pair& kv, valueDict) {
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

void ConfigObject::RestoreAttribute(const String& attr)
{
	Type::Ptr type = GetReflectionType();

	std::vector<String> tokens;
	boost::algorithm::split(tokens, attr, boost::is_any_of("."));

	String fieldName = tokens[0];

	int fid = type->GetFieldId(fieldName);
	Field field = type->GetFieldInfo(fid);

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
			BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot restore non-existing object attribute"));

		String prefix = tokens[0];

		for (std::vector<String>::size_type i = 1; i < tokens.size() - 1; i++) {
			if (!current.IsObjectType<Dictionary>())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Value must be a dictionary."));

			Dictionary::Ptr dict = current;

			const String& key = tokens[i];
			prefix += "." + key;

			if (!dict->Contains(key))
				BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot restore non-existing object attribute"));

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
			BOOST_FOREACH(const Dictionary::Pair& kv, original_attributes) {
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

		BOOST_FOREACH(const String& attr, restoredAttrs)
			original_attributes->Remove(attr);


	} else {
		newValue = oldValue;
	}

	original_attributes->Remove(attr);
	SetField(fid, newValue);

	SetVersion(Utility::GetTime());
}

bool ConfigObject::IsAttributeModified(const String& attr) const
{
	Dictionary::Ptr original_attributes = GetOriginalAttributes();

	if (!original_attributes)
		return false;

	return original_attributes->Contains(attr);
}

void ConfigObject::Register(void)
{
	ASSERT(!OwnsLock());

	ConfigType::Ptr dtype = GetType();
	dtype->RegisterObject(this);
}

void ConfigObject::Unregister(void)
{
	ASSERT(!OwnsLock());

	ConfigType::Ptr dtype = GetType();
	dtype->UnregisterObject(this);
}

void ConfigObject::Start(void)
{
	ObjectImpl<ConfigObject>::Start();

	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStartCalled(true);
}

void ConfigObject::Activate(void)
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

void ConfigObject::Stop(void)
{
	ObjectImpl<ConfigObject>::Stop();

	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStopCalled(true);
}

void ConfigObject::Deactivate(void)
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

void ConfigObject::OnConfigLoaded(void)
{
	/* Nothing to do here. */
}

void ConfigObject::OnAllConfigLoaded(void)
{
	/* Nothing to do here. */
}

void ConfigObject::CreateChildObjects(const Type::Ptr& childType)
{
	/* Nothing to do here. */
}

void ConfigObject::OnStateLoaded(void)
{
	/* Nothing to do here. */
}

void ConfigObject::Pause(void)
{
	SetPauseCalled(true);
}

void ConfigObject::Resume(void)
{
	SetResumeCalled(true);
}

void ConfigObject::SetAuthority(bool authority)
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

void ConfigObject::DumpObjects(const String& filename, int attributeTypes)
{
	Log(LogInformation, "ConfigObject")
	    << "Dumping program state to file '" << filename << "'";

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	BOOST_FOREACH(const ConfigType::Ptr& type, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, type->GetObjects()) {
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

	ConfigType::Ptr dt = ConfigType::GetByName(type);

	if (!dt)
		return;

	String name = persistentObject->Get("name");

	ConfigObject::Ptr object = dt->GetObject(name);

	if (!object)
		return;

	ASSERT(!object->IsActive());
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

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		upq.Enqueue(boost::bind(&ConfigObject::RestoreObject, message, attributeTypes));
		restored++;
	}

	sfp->Close();

	upq.Join();

	unsigned long no_state = 0;

	BOOST_FOREACH(const ConfigType::Ptr& type, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, type->GetObjects()) {
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

void ConfigObject::StopObjects(void)
{
	BOOST_FOREACH(const ConfigType::Ptr& dt, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, dt->GetObjects()) {
			object->Deactivate();
		}
	}
}

void ConfigObject::DumpModifiedAttributes(const boost::function<void(const ConfigObject::Ptr&, const String&, const Value&)>& callback)
{
	BOOST_FOREACH(const ConfigType::Ptr& dt, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, dt->GetObjects()) {
			Dictionary::Ptr originalAttributes = object->GetOriginalAttributes();

			if (!originalAttributes)
				continue;

			ObjectLock olock(originalAttributes);
			BOOST_FOREACH(const Dictionary::Pair& kv, originalAttributes) {
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
	ConfigType::Ptr dtype = ConfigType::GetByName(type);
	if (!dtype)
		return ConfigObject::Ptr();
	return dtype->GetObject(name);
}
