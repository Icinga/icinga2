#include "i2-base.h"

using namespace icinga;

void ConfigHive::AddObject(const ConfigObject::Ptr& object)
{
	string type = object->GetType();
	TypeIterator ti = Objects.find(type);

	if (ti == Objects.end()) {
		Objects[type] = map<string, ConfigObject::Ptr>();
		ti = Objects.find(type);
	}

	object->SetHive(static_pointer_cast<ConfigHive>(shared_from_this()));

	string name = object->GetName();
	ti->second[name] = object;

	ConfigHiveEventArgs::Ptr ea = new_object<ConfigHiveEventArgs>();
	ea->Source = shared_from_this();
	ea->Object = object;
	OnObjectCreated(ea);
}

void ConfigHive::RemoveObject(const ConfigObject::Ptr& object)
{
	string type = object->GetType();
	TypeIterator ti = Objects.find(type);

	if (ti == Objects.end())
		return;

	ti->second.erase(object->GetName());

	ConfigHiveEventArgs::Ptr ea = new_object<ConfigHiveEventArgs>();
	ea->Source = shared_from_this();
	ea->Object = object;
	OnObjectRemoved(ea);
}

ConfigObject::Ptr ConfigHive::GetObject(const string& type, const string& name)
{
	ConfigHive::TypeIterator ti = Objects.find(type);

	if (ti == Objects.end())
		return ConfigObject::Ptr();

	ConfigHive::ObjectIterator oi = ti->second.find(name);

	if (oi == ti->second.end())
		return ConfigObject::Ptr();

	return oi->second;
}
