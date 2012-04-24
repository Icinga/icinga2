#ifndef JSONRPCSERVER_H
#define JSONRPCSERVER_H

namespace icinga
{

class I2_JSONRPC_API JsonRpcServer : public TCPServer
{
public:
	typedef shared_ptr<JsonRpcServer> Ptr;
	typedef weak_ptr<JsonRpcServer> WeakPtr;

	JsonRpcServer(shared_ptr<SSL_CTX> sslContext);
};

}

#endif /* JSONRPCSERVER_H */
