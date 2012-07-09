#include "i2-cib.h"

using namespace icinga;

ServiceGroup::ServiceGroup(const ConfigObject::Ptr& configObject)
	: ConfigObjectAdapter(configObject)
{
	assert(GetType() == "servicegroup");
}


string ServiceGroup::GetAlias(void) const
{
	string value;

	if (GetProperty("alias", &value))
		return value;

	return GetName();
}

string ServiceGroup::GetNotesUrl(void) const
{
	string value;
	GetProperty("notes_url", &value);
	return value;
}

string ServiceGroup::GetActionUrl(void) const
{
	string value;
	GetProperty("action_url", &value);
	return value;
}

bool ServiceGroup::Exists(const string& name)
{
	return (ConfigObject::GetObject("hostgroup", name));
}

ServiceGroup ServiceGroup::GetByName(const string& name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("hostgroup", name);

	if (!configObject)
		throw invalid_argument("ServiceGroup '" + name + "' does not exist.");

	return ServiceGroup(configObject);
}

