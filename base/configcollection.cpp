#include "i2-base.h"

using namespace icinga;

/**
 * SetHive
 *
 * Sets the hive this collection belongs to.
 *
 * @param hive The hive.
 */
void ConfigCollection::SetHive(const ConfigHive::WeakPtr& hive)
{
	m_Hive = hive;
}

/**
 * GetHive
 *
 * Retrieves the hive this collection belongs to.
 *
 * @returns The hive.
 */
ConfigHive::WeakPtr ConfigCollection::GetHive(void) const
{
	return m_Hive;
}

/**
 * AddObject
 *
 * Adds a new object to this collection.
 *
 * @param object The new object.
 */
void ConfigCollection::AddObject(const ConfigObject::Ptr& object)
{
	RemoveObject(object);

	Objects[object->GetName()] = object;

	EventArgs ea;
	ea.Source = object;
	OnObjectCreated(ea);

	ConfigHive::Ptr hive = m_Hive.lock();
	if (hive)
		hive->OnObjectCreated(ea);
}

/**
 * RemoveObject
 *
 * Removes an object from this collection
 *
 * @param object The object that is to be removed.
 */
void ConfigCollection::RemoveObject(const ConfigObject::Ptr& object)
{
	ObjectIterator oi = Objects.find(object->GetName());

	if (oi != Objects.end()) {
		Objects.erase(oi);

		EventArgs ea;
		ea.Source = object;
		OnObjectRemoved(ea);

		ConfigHive::Ptr hive = m_Hive.lock();
		if (hive)
			hive->OnObjectRemoved(ea);
	}
}

/**
 * GetObject
 *
 * Retrieves an object by name.
 *
 * @param name The name of the object.
 * @returns The object or a null pointer if the specified object
 *          could not be found.
 */
ConfigObject::Ptr ConfigCollection::GetObject(const string& name) const
{
	ObjectConstIterator oi = Objects.find(name);

	if (oi == Objects.end())
		return ConfigObject::Ptr();

	return oi->second;
}

/**
 * ForEachObject
 *
 * Invokes the specified callback for each object contained in this collection.
 *
 * @param callback The callback.
 */
void ConfigCollection::ForEachObject(function<int (const EventArgs&)> callback)
{
	EventArgs ea;

	for (ObjectIterator oi = Objects.begin(); oi != Objects.end(); oi++) {
		ea.Source = oi->second;
		callback(ea);
	}
}
