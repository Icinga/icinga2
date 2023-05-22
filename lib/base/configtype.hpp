/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/type.hpp"
#include "base/dictionary.hpp"
#include <shared_mutex>
#include <unordered_map>

namespace icinga
{

class ConfigObject;

class ConfigType
{
public:
	virtual ~ConfigType();

	intrusive_ptr<ConfigObject> GetObject(const String& name) const;

	void RegisterObject(const intrusive_ptr<ConfigObject>& object);
	void UnregisterObject(const intrusive_ptr<ConfigObject>& object);

	std::vector<intrusive_ptr<ConfigObject> > GetObjects() const;

	template<typename T>
	static TypeImpl<T> *Get()
	{
		typedef TypeImpl<T> ObjType;
		return static_cast<ObjType *>(T::TypeInstance.get());
	}

	template<typename T>
	static std::vector<intrusive_ptr<T> > GetObjectsByType()
	{
		std::vector<intrusive_ptr<ConfigObject> > objects = GetObjectsHelper(T::TypeInstance.get());
		std::vector<intrusive_ptr<T> > result;
		result.reserve(objects.size());
for (const auto& object : objects) {
			result.push_back(static_pointer_cast<T>(object));
		}
		return result;
	}

	int GetObjectCount() const;

private:
	typedef std::unordered_map<String, intrusive_ptr<ConfigObject> > ObjectMap;
	typedef std::vector<intrusive_ptr<ConfigObject> > ObjectVector;

	mutable std::shared_timed_mutex m_Mutex;
	ObjectMap m_ObjectMap;
	ObjectVector m_ObjectVector;

	static std::vector<intrusive_ptr<ConfigObject> > GetObjectsHelper(Type *type);
};

}
