#ifndef JSONRPCSERVER_H
#define JSONRPCSERVER_H

namespace icinga
{

class JsonRpcServer : public TCPServer
{
public:
	typedef shared_ptr<JsonRpcServer> Ptr;
	typedef weak_ptr<JsonRpcServer> WeakPtr;

	JsonRpcServer(void);
};

}

#endif /* JSONRPCSERVER_H */
