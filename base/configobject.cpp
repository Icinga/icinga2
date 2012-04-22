#include "i2-base.h"

using namespace icinga;

ConfigObject::ConfigObject(const string& type, const string& name)
{
	m_Type = type;
	m_Name = name;
	m_Replicated = false;
}

void ConfigObject::SetHive(const ConfigHive::WeakPtr& hive)
{
	if (m_Hive.lock())
		throw InvalidArgumentException();

	m_Hive = hive;
	OnPropertyChanged += bind_weak(&ConfigObject::PropertyChangedHandler, shared_from_this());
}

ConfigHive::WeakPtr ConfigObject::GetHive(void) const
{
	return m_Hive;
}

void ConfigObject::SetName(const string& name)
{
	m_Name = name;
}

string ConfigObject::GetName(void) const
{
	return m_Name;
}

void ConfigObject::SetType(const string& type)
{
	m_Type = type;
}

string ConfigObject::GetType(void) const
{
	return m_Type;
}

void ConfigObject::SetReplicated(bool replicated)
{
	m_Replicated = replicated;
}

bool ConfigObject::GetReplicated(void) const
{
	return m_Replicated;
}

int ConfigObject::PropertyChangedHandler(const PropertyChangedEventArgs& dpcea)
{
	ConfigHive::Ptr hive = m_Hive.lock();
	if (hive) {
		hive->GetCollection(m_Type)->OnPropertyChanged(dpcea);
		hive->OnPropertyChanged(dpcea);
	}

	return 0;
}
