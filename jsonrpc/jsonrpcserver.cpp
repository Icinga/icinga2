#include "i2-jsonrpc.h"

using namespace icinga;

JsonRpcServer::JsonRpcServer(void)
{
	SetClientFactory(factory<JsonRpcClient>);
}
