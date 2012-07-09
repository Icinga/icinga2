#ifndef SERVICEGROUP_H
#define SERVICEGROUP_H

namespace icinga
{

class I2_CIB_API ServiceGroup : public ConfigObjectAdapter
{
public:
	ServiceGroup(const ConfigObject::Ptr& configObject);

	static bool Exists(const string& name);
	static ServiceGroup GetByName(const string& name);

	string GetAlias(void) const;
	string GetNotesUrl(void) const;
	string GetActionUrl(void) const;
};

}

#endif /* SERVICEGROUP_H */
