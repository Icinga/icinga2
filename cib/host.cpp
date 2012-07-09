#include "i2-cib.h"

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

set<string> Host::GetParents(void) const
{
	set<string> parents;

	Dictionary::Ptr dependencies;

	if (GetConfigObject()->GetProperty("dependencies", &dependencies)) {
		dependencies = Service::ResolveDependencies(*this, dependencies);

		Dictionary::Iterator it;
		for (it = dependencies->Begin(); it != dependencies->End(); it++) {
			Service service = Service::GetByName(it->second);

			string parent = service.GetHost().GetName();

			/* ignore ourselves */
			if (parent == GetName())
				continue;

			parents.insert(parent);
		}
	}

	return parents;
}

bool Host::IsReachable(void) const
{
	Dictionary::Ptr dependencies;
	if (GetConfigObject()->GetProperty("dependencies", &dependencies)) {
		dependencies = Service::ResolveDependencies(*this, dependencies);

		Dictionary::Iterator it;
		for (it = dependencies->Begin(); it != dependencies->End(); it++) {
			Service service = Service::GetByName(it->second);

			if (!service.IsReachable() ||
			    (service.GetState() != StateOK && service.GetState() != StateWarning)) {
				return false;
			}
		}
	}

	return true;
}

bool Host::IsUp(void) const
{
	Dictionary::Ptr hostchecks;
	if (GetConfigObject()->GetProperty("hostchecks", &hostchecks)) {
		hostchecks = Service::ResolveDependencies(*this, hostchecks);

		Dictionary::Iterator it;
		for (it = hostchecks->Begin(); it != hostchecks->End(); it++) {
			Service service = Service::GetByName(it->second);
			if (service.GetState() != StateOK && service.GetState() != StateWarning) {
				return false;
			}
		}
	}

	return true;
}
