#ifndef HOSTGROUP_H
#define HOSTGROUP_H

namespace icinga
{

class I2_ICINGA_API HostGroup : public ConfigObjectAdapter
{
public:
	HostGroup(const ConfigObject::Ptr& configObject)
		: ConfigObjectAdapter(configObject)
	{ }

	static bool Exists(const string& name);
	static HostGroup GetByName(const string& name);

	string GetAlias(void) const;
	string GetNotesUrl(void) const;
	string GetActionUrl(void) const;
};

}

#endif /* HOSTGROUP_H */
