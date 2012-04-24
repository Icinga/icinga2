#ifndef CONFIGCOLLECTION_H
#define CONFIGCOLLECTION_H

namespace icinga
{

class ConfigHive;

class I2_BASE_API ConfigCollection : public Object
{
private:
	weak_ptr<ConfigHive> m_Hive;

public:
	typedef shared_ptr<ConfigCollection> Ptr;
	typedef weak_ptr<ConfigCollection> WeakPtr;

	typedef map<string, ConfigObject::Ptr>::iterator ObjectIterator;
	typedef map<string, ConfigObject::Ptr>::const_iterator ObjectConstIterator;
	map<string, ConfigObject::Ptr> Objects;

	void SetHive(const weak_ptr<ConfigHive>& hive);
	weak_ptr<ConfigHive> GetHive(void) const;

	void AddObject(const ConfigObject::Ptr& object);
	void RemoveObject(const ConfigObject::Ptr& object);
	ConfigObject::Ptr GetObject(const string& name = string()) const;

	void ForEachObject(function<int (const EventArgs&)> callback);

	Event<EventArgs> OnObjectCreated;
	Event<EventArgs> OnObjectRemoved;
	Event<PropertyChangedEventArgs> OnPropertyChanged;
};

}

#endif /* CONFIGCOLLECTION_H */
