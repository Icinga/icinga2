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

#ifndef I2COMPATIDO_H
#define I2COMPATIDO_H

/**
 * @defgroup compatido Compat IDO component
 *
 * The compat ido component dumps config, status, history
 * to a socket where ido2db is listening
 */

#include <i2-base.h>
#include <i2-jsonrpc.h>
#include <i2-icinga.h>
#include <i2-cib.h>

using std::stringstream;

#include "idoprotoapi.h"
#include "idosocket.h"
#include "compatidocomponent.h"


#include <sstream>

#endif /* I2COMPATIDO_H */
