#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

class I2_ICINGA_API IcingaApplication : public Application
{
private:
	EndpointManager::Ptr m_EndpointManager;

	string m_PrivateKeyFile;
	string m_PublicKeyFile;
	string m_CAKeyFile;
	string m_Node;
	string m_Service;

	int NewComponentHandler(const EventArgs& ea);
	int DeletedComponentHandler(const EventArgs& ea);

	int NewIcingaConfigHandler(const EventArgs& ea);
	int DeletedIcingaConfigHandler(const EventArgs& ea);

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

	void SetPrivateKeyFile(string privkey);
	string GetPrivateKeyFile(void) const;

	void SetPublicKeyFile(string pubkey);
	string GetPublicKeyFile(void) const;

	void SetCAKeyFile(string cakey);
	string GetCAKeyFile(void) const;

	void SetNode(string node);
	string GetNode(void) const;

	void SetService(string service);
	string GetService(void) const;
};

}

#endif /* ICINGAAPPLICATION_H */
