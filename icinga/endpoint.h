#ifndef ENDPOINT_H
#define ENDPOINT_H

namespace icinga
{

class EndpointManager;

class I2_ICINGA_API Endpoint : public Object
{
private:
	bool m_Connected;

public:
	typedef shared_ptr<Endpoint> Ptr;
	typedef weak_ptr<Endpoint> WeakPtr;

	Endpoint(void);

	virtual void SetConnected(bool connected);
	virtual bool GetConnected(void);

	virtual void SendMessage(Endpoint::Ptr source, JsonRpcMessage::Ptr message) = 0;
};

}

#endif /* ENDPOINT_H */
