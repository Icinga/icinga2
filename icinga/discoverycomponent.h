#ifndef DISCOVERYCOMPONENT_H
#define DISCOVERYCOMPONENT_H

namespace icinga
{

class DiscoveryComponent : public IcingaComponent
{
private:
	VirtualEndpoint::Ptr m_DiscoveryEndpoint;

	IcingaApplication::Ptr GetIcingaApplication(void) const;

	int WelcomeMessageHandler(const NewRequestEventArgs& neea);
	int GetPeersMessageHandler(const NewRequestEventArgs& nrea);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* DISCOVERYCOMPONENT_H */
