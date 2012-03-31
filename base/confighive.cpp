#include "i2-base.h"

using namespace icinga;

void ConfigHive::AddObject(ConfigObject::RefType object)
{
	string type = object->GetType();
	TypeIterator ti = Objects.find(type);

	if (ti == Objects.end()) {
		Objects[type] = map<string, ConfigObject::RefType>();
		ti = Objects.find(type);
	}

	object->SetHive(static_pointer_cast<ConfigHive>(shared_from_this()));

	string name = object->GetName();
	ti->second[name] = object;

	ConfigHiveEventArgs::RefType ea = new_object<ConfigHiveEventArgs>();
	ea->Source = shared_from_this();
	ea->ConfigObject = object;
	OnObjectCreated(ea);
}

void ConfigHive::RemoveObject(ConfigObject::RefType object)
{
	string type = object->GetType();
	TypeIterator ti = Objects.find(type);

	if (ti == Objects.end())
		return;

	ti->second.erase(object->GetName());

	ConfigHiveEventArgs::RefType ea = new_object<ConfigHiveEventArgs>();
	ea->Source = shared_from_this();
	ea->ConfigObject = object;
	OnObjectRemoved(ea);
}

ConfigObject::RefType ConfigHive::GetObject(const string& type, const string& name)
{
	ConfigHive::TypeIterator ti = Objects.find(type);

	if (ti == Objects.end())
		return ConfigObject::RefType();

	ConfigHive::ObjectIterator oi = ti->second.find(name);

	if (oi == ti->second.end())
		return ConfigObject::RefType();

	return oi->second;
}
