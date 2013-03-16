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

#include "base/i2-base.h"
#include "remoting/i2-remoting.h"
#include "icinga/i2-icinga.h"

using namespace icinga;

#include "livestatus/connection.h"
#include "livestatus/column.h"
#include "livestatus/table.h"
#include "livestatus/filter.h"
#include "livestatus/combinerfilter.h"
#include "livestatus/orfilter.h"
#include "livestatus/andfilter.h"
#include "livestatus/negatefilter.h"
#include "livestatus/attributefilter.h"
#include "livestatus/query.h"
#include "livestatus/statustable.h"
#include "livestatus/contactgroupstable.h"
#include "livestatus/contactstable.h"
#include "livestatus/hoststable.h"
#include "livestatus/servicestable.h"
#include "livestatus/commentstable.h"
#include "livestatus/downtimestable.h"
#include "livestatus/component.h"

#endif /* I2LIVESTATUS_H */
