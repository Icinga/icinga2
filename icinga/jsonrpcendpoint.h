#ifndef JSONRPCENDPOINT_H
#define JSONRPCENDPOINT_H

namespace icinga
{

class I2_ICINGA_API JsonRpcEndpoint : public Endpoint
{
private:
	JsonRpcClient::Ptr m_Client;

	bool IsConnected(void) const;

public:
	JsonRpcEndpoint(void);

	JsonRpcClient::Ptr GetClient(void);
	void SetClient(JsonRpcClient::Ptr client);

	virtual void SendRequest(Endpoint::Ptr sender, JsonRpcRequest::Ptr message);
	virtual void SendResponse(Endpoint::Ptr sender, JsonRpcResponse::Ptr message);
};

}

#endif /* JSONRPCENDPOINT_H */
