#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include <map>

namespace icinga
{

class ConfigHive;

struct ConfigObjectEventArgs : public EventArgs
{
	typedef shared_ptr<ConfigObjectEventArgs> Ptr;
	typedef weak_ptr<ConfigObjectEventArgs> WeakPtr;

	string Property;
	string OldValue;
};

class ConfigObject : public Object
{
private:
	weak_ptr<ConfigHive> m_Hive;

	string m_Name;
	string m_Type;

public:
	typedef shared_ptr<ConfigObject> Ptr;
	typedef weak_ptr<ConfigObject> WeakPtr;

	typedef map<string, string>::iterator ParameterIterator;
	map<string, string> Properties;

	ConfigObject(const string& type, const string& name);

	void SetHive(const weak_ptr<ConfigHive>& hive);
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

#endif /* CONFIGOBJECT_H */
