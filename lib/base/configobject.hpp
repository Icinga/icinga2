// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include "base/i2-base.hpp"
#include "base/configobject-ti.hpp"
#include "base/object.hpp"
#include "base/type.hpp"
#include "base/dictionary.hpp"
#include <boost/signals2.hpp>

namespace icinga
{

class ConfigType;

/**
 * A dynamic object that can be instantiated from the configuration file.
 *
 * @ingroup base
 */
class ConfigObject : public ObjectImpl<ConfigObject>
{
public:
	DECLARE_OBJECT(ConfigObject);

	static boost::signals2::signal<void (const ConfigObject::Ptr&)> OnStateChanged;

	bool IsActive() const;
	bool IsPaused() const;

	void SetExtension(const String& key, const Value& value);
	Value GetExtension(const String& key);
	void ClearExtension(const String& key);

	ConfigObject::Ptr GetZone() const;

	void ModifyAttribute(const String& attr, const Value& value, bool updateVersion = true);
	void RestoreAttribute(const String& attr, bool updateVersion = true);
	bool IsAttributeModified(const String& attr) const;

	void Register();
	void Unregister();

	void PreActivate();
	void Activate(bool runtimeCreated = false, const Value& cookie = Empty);
	void Deactivate(bool runtimeRemoved = false, const Value& cookie = Empty);
	void SetAuthority(bool authority);

	void Start(bool runtimeCreated = false) override;
	void Stop(bool runtimeRemoved = false) override;

	virtual void Pause();
	virtual void Resume();

	virtual void OnConfigLoaded();
	virtual void CreateChildObjects(const Type::Ptr& childType);
	virtual void OnAllConfigLoaded();
	virtual void OnStateLoaded();

	Dictionary::Ptr GetSourceLocation() const override;

	template<typename T>
	static intrusive_ptr<T> GetObject(const String& name)
	{
		typedef TypeImpl<T> ObjType;
		auto *ptype = static_cast<ObjType *>(T::TypeInstance.get());
		return static_pointer_cast<T>(ptype->GetObject(name));
	}

	static ConfigObject::Ptr GetObject(const String& type, const String& name);

	static void DumpObjects(const String& filename, int attributeTypes = FAState);
	static void RestoreObjects(const String& filename, int attributeTypes = FAState);
	static void StopObjects();

	static void DumpModifiedAttributes(const std::function<void(const ConfigObject::Ptr&, const String&, const Value&)>& callback);

	static Object::Ptr GetPrototype();

private:
	ConfigObject::Ptr m_Zone;

	static void RestoreObject(const String& message, int attributeTypes);
};

#define DECLARE_OBJECTNAME(klass)						\
	inline static String GetTypeName()					\
	{									\
		return #klass;							\
	}									\
										\
	inline static intrusive_ptr<klass> GetByName(const String& name)	\
	{									\
		return ConfigObject::GetObject<klass>(name);			\
	}

}

#endif /* CONFIGOBJECT_H */
