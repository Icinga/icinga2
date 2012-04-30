#include "i2-base.h"

using namespace icinga;

/**
 * ConfigObject
 *
 * Constructor for the ConfigObject class.
 *
 * @param type The type of the object.
 * @param name The name of the object.
 */
ConfigObject::ConfigObject(const string& type, const string& name)
{
	m_Type = type;
	m_Name = name;
	m_Replicated = false;
}

/**
 * SetHive
 *
 * Sets the hive this object belongs to.
 *
 * @param hive The hive.
 */
void ConfigObject::SetHive(const ConfigHive::WeakPtr& hive)
{
	if (m_Hive.lock())
		throw InvalidArgumentException("Config object already has a parent hive.");

	m_Hive = hive;
	OnPropertyChanged += bind_weak(&ConfigObject::PropertyChangedHandler, shared_from_this());
}

/**
 * GetHive
 *
 * Retrieves the hive this object belongs to.
 *
 * @returns The hive.
 */
ConfigHive::WeakPtr ConfigObject::GetHive(void) const
{
	return m_Hive;
}

/**
 * SetName
 *
 * Sets the name of this object.
 *
 * @param name The name.
 */
void ConfigObject::SetName(const string& name)
{
	m_Name = name;
}

/**
 * GetName
 *
 * Retrieves the name of this object.
 *
 * @returns The name.
 */
string ConfigObject::GetName(void) const
{
	return m_Name;
}

/**
 * SetType
 *
 * Sets the type of this object.
 *
 * @param type The type.
 */
void ConfigObject::SetType(const string& type)
{
	m_Type = type;
}

/**
 * GetType
 *
 * Retrieves the type of this object.
 *
 * @returns The type.
 */
string ConfigObject::GetType(void) const
{
	return m_Type;
}

/**
 * SetReplicated
 *
 * Sets whether this object was replicated.
 *
 * @param replicated Whether this object was replicated.
 */
void ConfigObject::SetReplicated(bool replicated)
{
	m_Replicated = replicated;
}

/**
 * GetReplicated
 *
 * Retrieves whether this object was replicated.
 *
 * @returns Whether this object was replicated.
 */
bool ConfigObject::GetReplicated(void) const
{
	return m_Replicated;
}

/**
 * PropertyChangedHandler
 *
 * Handles changed properties by propagating them to the hive
 * and collection this object is contained in.
 *
 * @param dpcea The event arguments.
 * @returns 0.
 */
int ConfigObject::PropertyChangedHandler(const PropertyChangedEventArgs& dpcea)
{
	ConfigHive::Ptr hive = m_Hive.lock();
	if (hive) {
		hive->GetCollection(m_Type)->OnPropertyChanged(dpcea);
		hive->OnPropertyChanged(dpcea);
	}

	return 0;
}
