#ifndef HOST_H
#define HOST_H

namespace icinga
{

class I2_ICINGA_API Host : public ConfigObjectAdapter
{
public:
	Host(const ConfigObject::Ptr& configObject)
		: ConfigObjectAdapter(configObject)
	{ }

	static bool Exists(const string& name);
	static Host GetByName(const string& name);

	string GetAlias(void) const;
	Dictionary::Ptr GetGroups(void) const;
};

}

#endif /* HOST_H */
