#include "i2-icinga.h"

using namespace icinga;

string HostGroup::GetAlias(void) const
{
	string value;

	if (GetConfigObject()->GetProperty("alias", &value))
		return value;

	return GetName();
}

string HostGroup::GetNotesUrl(void) const
{
	string value;
	GetConfigObject()->GetProperty("notes_url", &value);
	return value;
}

string HostGroup::GetActionUrl(void) const
{
	string value;
	GetConfigObject()->GetProperty("action_url", &value);
	return value;
}

bool HostGroup::Exists(const string& name)
{
	return (ConfigObject::GetObject("hostgroup", name));
}

HostGroup HostGroup::GetByName(const string& name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("hostgroup", name);

	if (!configObject)
		throw invalid_argument("HostGroup '" + name + "' does not exist.");

	return HostGroup(configObject);
}

