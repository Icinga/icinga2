#include "i2-icinga.h"

using namespace icinga;

string Host::GetDisplayName(void) const
{
	string value;

	if (GetConfigObject()->GetProperty("displayname", &value))
		return value;

	return GetName();
}

Host Host::GetByName(string name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("host", name);

	if (!configObject)
		throw invalid_argument("Host '" + name + "' does not exist.");

	return Host(configObject);
}
