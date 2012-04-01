#ifndef I2_CONFIGOBJECT_H
#define I2_CONFIGOBJECT_H

#include <map>

namespace icinga
{

using std::map;
using std::string;

class ConfigHive;

class ConfigObject : public Object
{
private:
	weak_ptr<ConfigHive> m_Hive;

	string m_Name;
	string m_Type;

public:
	typedef shared_ptr<ConfigObject> RefType;
	typedef weak_ptr<ConfigObject> WeakRefType;

	typedef map<string, string>::iterator ParameterIterator;
	map<string, string> Properties;

	void SetHive(const weak_ptr<ConfigHive>& name);
	weak_ptr<ConfigHive> GetHive(void) const;

	void SetName(const string& name);
	string GetName(void) const;

	void SetType(const string& type);
	string GetType(void) const;

	void SetProperty(const string& name, const string& value);
	void SetPropertyInteger(const string& name, int value);
	void SetPropertyDouble(const string& name, double value);

	bool GetProperty(const string& name, string *value) const;
	bool GetPropertyInteger(const string& name, int *value) const;
	bool GetPropertyDouble(const string& name, double *value) const;
};

}

#endif /* I2_CONFIGOBJECT_H */
