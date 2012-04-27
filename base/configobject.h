#ifndef CONFIGOBJECT_H
#define CONFIGOBJECT_H

#include <map>

namespace icinga
{

class ConfigHive;

class I2_BASE_API ConfigObject : public Dictionary
{
private:
	weak_ptr<ConfigHive> m_Hive;

	string m_Name;
	string m_Type;
	bool m_Replicated;

	int PropertyChangedHandler(const PropertyChangedEventArgs& dpcea);

public:
	typedef shared_ptr<ConfigObject> Ptr;
	typedef weak_ptr<ConfigObject> WeakPtr;

	ConfigObject(const string& type, const string& name);

	void SetHive(const weak_ptr<ConfigHive>& hive);
	weak_ptr<ConfigHive> GetHive(void) const;

	void SetName(const string& name);
	string GetName(void) const;

	void SetType(const string& type);
	string GetType(void) const;

	void SetReplicated(bool replicated);
	bool GetReplicated(void) const;
};

}

#endif /* CONFIGOBJECT_H */
