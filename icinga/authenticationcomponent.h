#ifndef AUTHENTICATIONCOMPONENT_H
#define AUTHENTICATIONCOMPONENT_H

namespace icinga
{

class AuthenticationComponent : public IcingaComponent
{
private:
	VirtualEndpoint::Ptr m_AuthenticationEndpoint;

	int NewEndpointHandler(const NewEndpointEventArgs& neea);
	int IdentityMessageHandler(const NewRequestEventArgs& nrea);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* AUTHENTICATIONCOMPONENT_H */
