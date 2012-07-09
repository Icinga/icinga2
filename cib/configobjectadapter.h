#ifndef CONFIGOBJECTADAPTER_H
#define CONFIGOBJECTADAPTER_H

namespace icinga
{

class I2_CIB_API ConfigObjectAdapter
{
public:
	ConfigObjectAdapter(const ConfigObject::Ptr& configObject)
		: m_ConfigObject(configObject)
	{ }

	string GetType(void) const;
	string GetName(void) const;

	bool IsLocal(void) const;

	ConfigObject::Ptr GetConfigObject() const;

	template<typename T>
	bool GetProperty(const string& key, T *value) const
	{
		return GetConfigObject()->GetProperty(key, value);
	}

	template<typename T>
	void SetTag(const string& key, const T& value)
	{
		GetConfigObject()->SetTag(key, value);
	}

	template<typename T>
	bool GetTag(const string& key, T *value) const
	{
		return GetConfigObject()->GetTag(key, value);
	}

private:
	ConfigObject::Ptr m_ConfigObject;
};

}

#endif /* CONFIGOBJECTADAPTER_H */
