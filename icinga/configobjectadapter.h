#ifndef CONFIGOBJECTADAPTER_H
#define CONFIGOBJECTADAPTER_H

namespace icinga
{

class I2_ICINGA_API ConfigObjectAdapter
{
public:
	ConfigObjectAdapter(const ConfigObject::Ptr& configObject)
		: m_ConfigObject(configObject)
	{ }

	string GetType(void) const;
	string GetName(void) const;

	bool IsLocal(void) const;

protected:
	ConfigObject::Ptr GetConfigObject() const;

private:
	ConfigObject::Ptr m_ConfigObject;
};

}

#endif /* CONFIGOBJECTADAPTER_H */