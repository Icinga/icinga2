#ifndef CONFIGCOLLECTION_H
#define CONFIGCOLLECTION_H

namespace icinga
{

class ConfigHive;

class ConfigCollection : public Object
{
private:
	weak_ptr<ConfigHive> m_Hive;

public:
	typedef shared_ptr<ConfigCollection> Ptr;
	typedef weak_ptr<ConfigCollection> WeakPtr;

	typedef map<string, ConfigObject::Ptr>::iterator ObjectIterator;
	map<string, ConfigObject::Ptr> Objects;

	void SetHive(const weak_ptr<ConfigHive>& hive);
	weak_ptr<ConfigHive> GetHive(void) const;

	void AddObject(const ConfigObject::Ptr& object);
	void RemoveObject(const ConfigObject::Ptr& object);
	ConfigObject::Ptr GetObject(const string& name = string());

	void ForEachObject(function<int (ConfigObjectEventArgs::Ptr)> callback);

	event<ConfigObjectEventArgs::Ptr> OnObjectCreated;
	event<ConfigObjectEventArgs::Ptr> OnObjectRemoved;
	event<ConfigObjectEventArgs::Ptr> OnPropertyChanged;
};

}

#endif /* CONFIGCOLLECTION_H */
