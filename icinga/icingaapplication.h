#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

class I2_ICINGA_API IcingaApplication : public Application
{
private:
	EndpointManager::Ptr m_EndpointManager;

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

	int Main(const vector<string>& args);

	void PrintUsage(const string& programPath);

	EndpointManager::Ptr GetEndpointManager(void);
};

}

#endif /* ICINGAAPPLICATION_H */
