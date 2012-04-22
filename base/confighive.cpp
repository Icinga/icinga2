#include "i2-base.h"

using namespace icinga;

void ConfigHive::AddObject(const ConfigObject::Ptr& object)
{
	object->SetHive(static_pointer_cast<ConfigHive>(shared_from_this()));
	GetCollection(object->GetType())->AddObject(object);
}

void ConfigHive::RemoveObject(const ConfigObject::Ptr& object)
{
	GetCollection(object->GetType())->RemoveObject(object);
}

ConfigObject::Ptr ConfigHive::GetObject(const string& type, const string& name)
{
	return GetCollection(type)->GetObject(name);
}

ConfigCollection::Ptr ConfigHive::GetCollection(const string& collection)
{
	CollectionIterator ci = Collections.find(collection);

	if (ci == Collections.end()) {
		Collections[collection] = make_shared<ConfigCollection>();
		ci = Collections.find(collection);
	}

	return ci->second;
}

void ConfigHive::ForEachObject(const string& type,
    function<int (const EventArgs&)> callback)
{
	CollectionIterator ci = Collections.find(type);

	if (ci == Collections.end())
		return;

	ci->second->ForEachObject(callback);
}
