/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef I2CIB_H
#define I2CIB_H

/**
 * @defgroup cib Common Information Base
 *
 * The CIB component implements functionality to gather status
 * updates from all the other Icinga components.
 */

#include <i2-dyn.h>
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
#include "nagioschecktask.h"
#include "nullchecktask.h"

#include "checkresultmessage.h"

#include "cib.h"

#endif /* I2CIB_H */
