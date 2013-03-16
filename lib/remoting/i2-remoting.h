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

#ifndef I2REMOTING_H
#define I2REMOTING_H

/**
 * @defgroup remoting Remoting library
 *
 * Implements server and client classes for the JSON-RPC protocol. Also
 * supports endpoint-based communication using messages.
 */

#include "base/i2-base.h"
#include "config/i2-config.h"

#ifdef I2_REMOTING_BUILD
#	define I2_REMOTING_API I2_EXPORT
#else /* I2_REMOTING_BUILD */
#	define I2_REMOTING_API I2_IMPORT
#endif /* I2_REMOTING_BUILD */

#include "messagepart.h"
#include "requestmessage.h"
#include "responsemessage.h"
#include "jsonrpcconnection.h"
#include "endpoint.h"
#include "endpointmanager.h"

#endif /* I2REMOTING_H */
