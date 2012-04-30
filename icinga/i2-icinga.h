#ifndef I2ICINGA_H
#define I2ICINGA_H

#include <i2-base.h>
#include <i2-jsonrpc.h>
#include <set>

#ifdef I2_ICINGA_BUILD
#	define I2_ICINGA_API I2_EXPORT
#else /* I2_ICINGA_BUILD */
#	define I2_ICINGA_API I2_IMPORT
#endif /* I2_ICINGA_BUILD */

#include "endpoint.h"
#include "jsonrpcendpoint.h"
#include "virtualendpoint.h"
#include "endpointmanager.h"
#include "icingaapplication.h"
#include "icingacomponent.h"

#endif /* I2ICINGA_H */
