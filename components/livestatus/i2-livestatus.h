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

#ifndef I2LIVESTATUS_H
#define I2LIVESTATUS_H

/**
 * @defgroup livestatus Livestatus component
 *
 * The livestatus component implements livestatus queries.
 */

#include <i2-base.h>
#include <i2-remoting.h>
#include <i2-icinga.h>

using namespace icinga;

#include "connection.h"
#include "column.h"
#include "table.h"
#include "filter.h"
#include "combinerfilter.h"
#include "orfilter.h"
#include "andfilter.h"
#include "negatefilter.h"
#include "attributefilter.h"
#include "query.h"
#include "statustable.h"
#include "contactgroupstable.h"
#include "contactstable.h"
#include "hoststable.h"
#include "component.h"

#endif /* I2LIVESTATUS_H */
