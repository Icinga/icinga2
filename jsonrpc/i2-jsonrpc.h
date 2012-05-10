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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef I2JSONRPC_H
#define I2JSONRPC_H

#include <map>
#include <i2-base.h>

#ifdef I2_JSONRPC_BUILD
#	define I2_JSONRPC_API I2_EXPORT
#else /* I2_JSONRPC_BUILD */
#	define I2_JSONRPC_API I2_IMPORT
#endif /* I2_JSONRPC_BUILD */

#include "variant.h"
#include "dictionary.h"
#include "message.h"
#include "netstring.h"
#include "jsonrpcrequest.h"
#include "jsonrpcresponse.h"
#include "jsonrpcclient.h"
#include "jsonrpcserver.h"

#endif /* I2JSONRPC_H */
