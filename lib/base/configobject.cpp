/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/configobject.hpp"
#include "base/configobject-ti.cpp"
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
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(ConfigObject, ConfigObject::GetPrototype());

boost::signals2::signal<void (const ConfigObject::Ptr&)> ConfigObject::OnStateChanged;

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
	bool ValidateName(const String& type, const String& name) const override
	{
		Type::Ptr ptype = Type::GetByName(type);
		auto *dtype = dynamic_cast<ConfigType *>(ptype.get());

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

	std::vector<String> tokens = attr.Split(".");

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
	ValidateField(fid, Lazy<Value>{newValue}, utils);

	SetField(fid, newValue);

	if (updateVersion && (field.Attributes & FAConfig))
		SetVersion(Utility::GetTime());

	if (updated_original_attributes)
		NotifyOriginalAttributes();
}

void ConfigObject::RestoreAttribute(const String& attr, bool updateVersion)
{
	Type::Ptr type = GetReflectionType();

	std::vector<String> tokens = attr.Split(".");

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
				std::vector<String> originalTokens = String(kv.first).Split(".");

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

void ConfigObject::Activate(bool runtimeCreated, const Value& cookie)
{
	CONTEXT("Activating object '" + GetName() + "' of type '" + GetReflectionType()->GetName() + "'");

	{
		ObjectLock olock(this);

		Start(runtimeCreated);

		ASSERT(GetStartCalled());

		if (GetHAMode() == HARunEverywhere)
			SetAuthority(true);
	}

	NotifyActive(cookie);
}

void ConfigObject::Stop(bool runtimeRemoved)
{
	ObjectImpl<ConfigObject>::Stop(runtimeRemoved);

	ObjectLock olock(this);

	SetStopCalled(true);
}

void ConfigObject::Deactivate(bool runtimeRemoved, const Value& cookie)
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

	NotifyActive(cookie);
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

	try {
		Utility::Glob(filename + ".tmp.*", &Utility::Remove, GlobFile);
	} catch (const std::exception& ex) {
		Log(LogWarning, "ConfigObject") << DiagnosticInformation(ex);
	}

	std::fstream fp;
	String tempFilename = Utility::CreateTempFile(filename + ".tmp.XXXXXX", 0600, fp);
	fp.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + tempFilename + "' file"));

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			Dictionary::Ptr update = Serialize(object, attributeTypes);

			if (!update)
				continue;

			Dictionary::Ptr persistentObject = new Dictionary({
				{ "type", type->GetName() },
				{ "name", object->GetName() },
				{ "update", update }
			});

			String json = JsonEncode(persistentObject);

			NetString::WriteStringToStream(sfp, json);
		}
	}

	sfp->Close();

	fp.close();

	Utility::RenameFile(tempFilename, filename);
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

	WorkQueue upq(25000, Configuration::Concurrency);
	upq.SetName("ConfigObject::RestoreObjects");

	String message;
	StreamReadContext src;
	for (;;) {
		StreamReadStatus srs = NetString::ReadStringFromStream(sfp, &message, src);

		if (srs == StatusEof)
			break;

		if (srs != StatusNewItem)
			continue;

		upq.Enqueue([message, attributeTypes]() { RestoreObject(message, attributeTypes); });
		restored++;
	}

	sfp->Close();

	upq.Join();

	unsigned long no_state = 0;

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

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
	std::vector<Type::Ptr> types = Type::GetAllTypes();

	std::sort(types.begin(), types.end(), [](const Type::Ptr& a, const Type::Ptr& b) {
		if (a->GetActivationPriority() > b->GetActivationPriority())
			return true;
		return false;
	});

	for (const Type::Ptr& type : types) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
#ifdef I2_DEBUG
			Log(LogDebug, "ConfigObject")
				<< "Deactivate() called for config object '" << object->GetName() << "' with type '" << type->GetName() << "'.";
#endif /* I2_DEBUG */
			object->Deactivate();
		}
	}
}

void ConfigObject::DumpModifiedAttributes(const std::function<void(const ConfigObject::Ptr&, const String&, const Value&)>& callback)
{
	for (const Type::Ptr& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

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

				std::vector<String> tokens = key.Split(".");

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
	auto *ctype = dynamic_cast<ConfigType *>(ptype.get());

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

	return new Dictionary({
		{ "path", di.Path },
		{ "first_line", di.FirstLine },
		{ "first_column", di.FirstColumn },
		{ "last_line", di.LastLine },
		{ "last_column", di.LastColumn }
	});
}

NameComposer::~NameComposer()
{ }
