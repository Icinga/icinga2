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

#ifndef I2ICINGA_H
#define I2ICINGA_H

/**
 * @defgroup icinga Icinga application
 *
 * The Icinga application is in charge of boot-strapping the Icinga
 * environment and loading additional components.
 */

#include <i2-base.h>
#include <i2-jsonrpc.h>
#include <set>

using boost::iterator_range;
using boost::algorithm::is_any_of;

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

#include "configobjectadapter.h"
#include "host.h"
#include "service.h"

#include "cib.h"

#include "macroprocessor.h"
#include "checkresult.h"
#include "checktask.h"
#include "nagioschecktask.h"

#endif /* I2ICINGA_H */
