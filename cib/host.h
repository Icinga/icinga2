#ifndef HOST_H
#define HOST_H

namespace icinga
{

class I2_CIB_API Host : public ConfigObjectAdapter
{
public:
	Host(const ConfigObject::Ptr& configObject)
		: ConfigObjectAdapter(configObject)
	{
		assert(GetType() == "host");
	}

	static bool Exists(const string& name);
	static Host GetByName(const string& name);

	string GetAlias(void) const;
	Dictionary::Ptr GetGroups(void) const;
	set<string> GetParents(void) const;

	bool IsReachable(void) const;
	bool IsUp(void) const;
};

}

#endif /* HOST_H */
