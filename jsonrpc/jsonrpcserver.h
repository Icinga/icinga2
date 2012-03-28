#ifndef I2_JSONRPCSERVER_H
#define I2_JSONRPCSERVER_H

namespace icinga
{

class JsonRpcServer : public TCPServer
{
public:
	typedef shared_ptr<JsonRpcServer> RefType;
	typedef weak_ptr<JsonRpcServer> WeakRefType;

	JsonRpcServer(void);
};

}

#endif /* I2_JSONRPCSERVER_H */