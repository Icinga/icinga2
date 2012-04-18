#include "i2-base.h"

using namespace icinga;

void ConfigCollection::SetHive(const ConfigHive::WeakPtr& hive)
{
	m_Hive = hive;
}

ConfigHive::WeakPtr ConfigCollection::GetHive(void) const
{
	return m_Hive;
}

void ConfigCollection::AddObject(const ConfigObject::Ptr& object)
{
	Objects[object->GetName()] = object;

	ConfigObjectEventArgs ea;
	ea.Source = object;
	OnObjectCreated(ea);

	ConfigHive::Ptr hive = m_Hive.lock();
	if (hive)
		hive->OnObjectCreated(ea);
}

void ConfigCollection::RemoveObject(const ConfigObject::Ptr& object)
{
	ObjectIterator oi = Objects.find(object->GetName());

	if (oi != Objects.end()) {
		Objects.erase(oi);

		ConfigObjectEventArgs ea;
		ea.Source = object;
		OnObjectRemoved(ea);

		ConfigHive::Ptr hive = m_Hive.lock();
		if (hive)
			hive->OnObjectRemoved(ea);
	}
}

ConfigObject::Ptr ConfigCollection::GetObject(const string& name)
{
	ObjectIterator oi = Objects.find(name);

	if (oi == Objects.end())
		return ConfigObject::Ptr();

	return oi->second;
}

void ConfigCollection::ForEachObject(function<int (const ConfigObjectEventArgs&)> callback)
{
	ConfigObjectEventArgs ea;

	for (ObjectIterator oi = Objects.begin(); oi != Objects.end(); oi++) {
		ea.Source = oi->second;
		callback(ea);
	}
}
