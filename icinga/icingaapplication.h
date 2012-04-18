#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

class I2_ICINGA_API IcingaApplication : public Application
{
private:
	EndpointManager::Ptr m_EndpointManager;
	Timer::Ptr m_TestTimer;
	VirtualEndpoint::Ptr m_TestEndpoint;

	int NewComponentHandler(const ConfigObjectEventArgs& ea);
	int DeletedComponentHandler(const ConfigObjectEventArgs& ea);

	int NewRpcListenerHandler(const ConfigObjectEventArgs& ea);
	int DeletedRpcListenerHandler(const ConfigObjectEventArgs& ea);

	int NewRpcConnectionHandler(const ConfigObjectEventArgs& ea);
	int DeletedRpcConnectionHandler(const ConfigObjectEventArgs& ea);

	int TestTimerHandler(const TimerEventArgs& tea);
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
