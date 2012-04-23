#ifndef DISCOVERYCOMPONENT_H
#define DISCOVERYCOMPONENT_H

namespace icinga
{

class DiscoveryComponent : public Component
{
private:
	VirtualEndpoint::Ptr m_DiscoveryEndpoint;

	IcingaApplication::Ptr GetIcingaApplication(void) const;

	int NewEndpointHandler(const NewEndpointEventArgs& neea);
	int IdentityChangedHandler(const EventArgs& neea);
	int WelcomeMessageHandler(const NewRequestEventArgs& nrea);
	int GetPeersMessageHandler(const NewRequestEventArgs& nrea);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* DISCOVERYCOMPONENT_H */
