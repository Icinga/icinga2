#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

class I2_ICINGA_API IcingaApplication : public Application
{
private:
	EndpointManager::Ptr m_EndpointManager;

	int NewComponentHandler(const EventArgs& ea);
	int DeletedComponentHandler(const EventArgs& ea);

	int NewRpcListenerHandler(const EventArgs& ea);
	int DeletedRpcListenerHandler(const EventArgs& ea);

	int NewRpcConnectionHandler(const EventArgs& ea);
	int DeletedRpcConnectionHandler(const EventArgs& ea);

	int TestTimerHandler(const TimerEventArgs& tea);
public:
	typedef shared_ptr<IcingaApplication> Ptr;
	typedef weak_ptr<IcingaApplication> WeakPtr;

	int Main(const vector<string>& args);

	void PrintUsage(const string& programPath);

	EndpointManager::Ptr GetEndpointManager(void);
};

}

#endif /* ICINGAAPPLICATION_H */
