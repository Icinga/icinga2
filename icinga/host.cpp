#include "i2-icinga.h"

using namespace icinga;

string Host::GetAlias(void) const
{
	string value;

	if (GetConfigObject()->GetProperty("alias", &value))
		return value;

	return GetName();
}

bool Host::Exists(const string& name)
{
	return (ConfigObject::GetObject("host", name));
}

Host Host::GetByName(const string& name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("host", name);

	if (!configObject)
		throw invalid_argument("Host '" + name + "' does not exist.");

	return Host(configObject);
}

Dictionary::Ptr Host::GetGroups(void) const
{
	Dictionary::Ptr value;
	GetConfigObject()->GetProperty("hostgroups", &value);
	return value;
}
