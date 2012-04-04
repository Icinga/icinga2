#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

class IcingaApplication : public Application
{
private:
	ConnectionManager::Ptr m_ConnectionManager;

	int NewComponentHandler(ConfigObjectEventArgs::Ptr ea);
	int DeletedComponentHandler(ConfigObjectEventArgs::Ptr ea);

	int NewRpcListenerHandler(ConfigObjectEventArgs::Ptr ea);
	int DeletedRpcListenerHandler(ConfigObjectEventArgs::Ptr ea);

	int NewRpcConnectionHandler(ConfigObjectEventArgs::Ptr ea);
	int DeletedRpcConnectionHandler(ConfigObjectEventArgs::Ptr ea);

public:
	typedef shared_ptr<IcingaApplication> Ptr;
	typedef weak_ptr<IcingaApplication> WeakPtr;

	IcingaApplication(void);

	virtual int Main(const vector<string>& args);

	void PrintUsage(const string& programPath);

	virtual ConnectionManager::Ptr GetConnectionManager(void);
};

}

#endif /* ICINGAAPPLICATION_H */
