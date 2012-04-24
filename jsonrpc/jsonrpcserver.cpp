#include "i2-jsonrpc.h"

using namespace icinga;

JsonRpcServer::JsonRpcServer(shared_ptr<SSL_CTX> sslContext)
{
	SetClientFactory(bind(&JsonRpcClientFactory, RoleInbound, sslContext));
}
