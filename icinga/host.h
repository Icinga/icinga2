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

	static Host GetByName(string name);

	string GetDisplayName(void) const;
};

}

#endif /* HOST_H */
