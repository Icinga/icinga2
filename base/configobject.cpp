#include "i2-base.h"

using namespace icinga;

void ConfigObject::SetHive(const ConfigHive::WeakPtr& hive)
{
	m_Hive = hive;
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

void ConfigObject::SetProperty(const string& name, const string& value)
{
	Properties[name] = value;

	ConfigHive::Ptr hive = m_Hive.lock();
	if (hive.get() != NULL) {
		ConfigHiveEventArgs::Ptr ea = make_shared<ConfigHiveEventArgs>();
		ea->Source = hive;
		ea->Object = static_pointer_cast<ConfigObject>(shared_from_this());
		ea->Property = name;

		string oldValue;
		if (GetProperty(name, &oldValue))
			ea->OldValue = oldValue;

		hive->OnPropertyChanged(ea);
	}
}

bool ConfigObject::GetProperty(const string& name, string *value) const
{
	map<string, string>::const_iterator vi = Properties.find(name);
	if (vi == Properties.end())
		return false;
	*value = vi->second;
	return true;
}

bool ConfigObject::GetPropertyInteger(const string& name, int *value) const
{
	string stringValue;
	if (!GetProperty(name, &stringValue))
		return false;
	*value = strtol(stringValue.c_str(), NULL, 10);
	return true;
}

bool ConfigObject::GetPropertyDouble(const string& name, double *value) const
{
	string stringValue;
	if (!GetProperty(name, &stringValue))
		return false;
	*value = strtod(stringValue.c_str(), NULL);
	return true;
}
