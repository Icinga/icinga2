#ifndef I2_SUBSCRIPTIONCOMPONENT_H
#define I2_SUBSCRIPTIONCOMPONENT_H

namespace icinga
{

class SubscriptionComponent : public Component
{
private:
	VirtualEndpoint::Ptr m_SubscriptionEndpoint;

	IcingaApplication::Ptr GetIcingaApplication(void) const;

	int NewEndpointHandler(const NewEndpointEventArgs& neea);
	int SubscribeMessageHandler(const NewRequestEventArgs& nrea);
	int ProvideMessageHandler(const NewRequestEventArgs& nrea);

	int SyncSubscription(Endpoint::Ptr target, string type, const NewMethodEventArgs& nmea);
	int SyncSubscriptions(Endpoint::Ptr target, const NewEndpointEventArgs& neea);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* I2_SUBSCRIPTIONCOMPONENT_H */
