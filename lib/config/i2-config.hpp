/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef I2CONFIG_H
#define I2CONFIG_H

/**
 * @defgroup config Configuration library
 *
 * The configuration library implements a compiler for Icinga 2's configuration
 * format. It also provides functionality for creating configuration objects
 * at runtime.
 */

#include "base/i2-base.hpp"

#ifdef I2_CONFIG_BUILD
#	define I2_CONFIG_API I2_EXPORT
#else /* I2_CONFIG_BUILD */
#	define I2_CONFIG_API I2_IMPORT
#endif /* I2_CONFIG_BUILD */

#endif /* I2CONFIG_H */
