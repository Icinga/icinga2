#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

class IcingaApplication : public Application
{
private:
	ConnectionManager::RefType m_ConnectionManager;

	int ConfigObjectCreatedHandler(ConfigHiveEventArgs::RefType ea);
	int ConfigObjectRemovedHandler(ConfigHiveEventArgs::RefType ea);

public:
	typedef shared_ptr<IcingaApplication> RefType;
	typedef weak_ptr<IcingaApplication> WeakRefType;

	IcingaApplication(void);

	virtual int Main(const vector<string>& args);

	void PrintUsage(const string& programPath);

	virtual ConnectionManager::RefType GetConnectionManager(void);
};

}

#endif /* ICINGAAPPLICATION_H */
