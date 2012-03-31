#include "i2-base.h"

using namespace icinga;

void ConfigObject::SetHive(const ConfigHive::WeakRefType& hive)
{
	m_Hive = hive;
}

ConfigHive::WeakRefType ConfigObject::GetHive(void) const
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
	string oldValue = GetProperty(name);

	Properties[name] = value;

	ConfigHive::RefType hive = m_Hive.lock();
	if (hive.get() != NULL) {
		ConfigHiveEventArgs::RefType ea = new_object<ConfigHiveEventArgs>();
		ea->Source = hive;
		ea->ConfigObject = static_pointer_cast<ConfigObject>(shared_from_this());
		ea->Property = name;
		ea->OldValue = oldValue;
		hive->OnPropertyChanged(ea);
	}
}

string ConfigObject::GetProperty(const string& name, const string& default) const
{
	map<string, string>::const_iterator vi = Properties.find(name);
	if (vi == Properties.end())
		return default;
	return vi->second;
}

int ConfigObject::GetPropertyInteger(const string& name, int default) const
{
	string value = GetProperty(name);
	if (value == string())
		return default;
	return strtol(value.c_str(), NULL, 10);
}

double ConfigObject::GetPropertyDouble(const string& name, double default) const
{
	string value = GetProperty(name);
	if (value == string())
		return default;
	return strtod(value.c_str(), NULL);
}
