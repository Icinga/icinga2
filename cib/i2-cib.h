#ifndef I2CIB_H
#define I2CIB_H

/**
 * @defgroup cib Common Information Base
 *
 * The CIB component implements functionality to gather status
 * updates from all the other Icinga components.
 */

#include <i2-icinga.h>

#ifdef I2_CIB_BUILD
#	define I2_CIB_API I2_EXPORT
#else /* I2_CIB_BUILD */
#	define I2_CIB_API I2_IMPORT
#endif /* I2_CIB_BUILD */

#include "configobjectadapter.h"
#include "host.h"
#include "hostgroup.h"
#include "service.h"
#include "servicegroup.h"

#include "macroprocessor.h"
#include "checkresult.h"
#include "checktask.h"
#include "nagioschecktask.h"

#include "servicestatusmessage.h"

#include "cib.h"

#endif /* I2CIB_H */
