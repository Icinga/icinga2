#ifndef JSONRPCSERVER_H
#define JSONRPCSERVER_H

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

#endif /* JSONRPCSERVER_H */
