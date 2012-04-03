#ifndef CONFIGHIVE_H
#define CONFIGHIVE_H

namespace icinga
{

struct ConfigHiveEventArgs : public EventArgs
{
	typedef shared_ptr<ConfigHiveEventArgs> Ptr;
	typedef weak_ptr<ConfigHiveEventArgs> WeakPtr;

	ConfigObject::Ptr Object;
	string Property;
	string OldValue;
};

class ConfigHive : public Object
{
public:
	typedef shared_ptr<ConfigHive> Ptr;
	typedef weak_ptr<ConfigHive> WeakPtr;

	typedef map< string, map<string, ConfigObject::Ptr> >::iterator TypeIterator;
	typedef map<string, ConfigObject::Ptr>::iterator ObjectIterator;
	map< string, map<string, ConfigObject::Ptr> > Objects;

	void AddObject(const ConfigObject::Ptr& object);
	void RemoveObject(const ConfigObject::Ptr& object);
	ConfigObject::Ptr GetObject(const string& type, const string& name = string());

	event<ConfigHiveEventArgs::Ptr> OnObjectCreated;
	event<ConfigHiveEventArgs::Ptr> OnObjectRemoved;
	event<ConfigHiveEventArgs::Ptr> OnPropertyChanged;
};

}

#endif /* CONFIGHIVE_H */
