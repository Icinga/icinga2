#ifndef I2JSONRPC_H
#define I2JSONRPC_H

#include <map>
#include <i2-base.h>
#include <cJSON.h>

#ifdef I2_JSONRPC_BUILD
#	define I2_JSONRPC_API I2_EXPORT
#else /* I2_JSONRPC_BUILD */
#	define I2_JSONRPC_API I2_IMPORT
#endif /* I2_JSONRPC_BUILD */

#include "message.h"
#include "netstring.h"
#include "jsonrpcrequest.h"
#include "jsonrpcresponse.h"
#include "jsonrpcclient.h"
#include "jsonrpcserver.h"

#endif /* I2JSONRPC_H */
