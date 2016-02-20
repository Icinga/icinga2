/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
 * @defgroup icinga Icinga library
 *
 * The Icinga library implements all Icinga-specific functionality that is
 * common to all components (e.g. hosts, services, etc.).
 */

#include "base/i2-base.hpp"

#ifdef I2_ICINGA_BUILD
#	define I2_ICINGA_API I2_EXPORT
#else /* I2_ICINGA_BUILD */
#	define I2_ICINGA_API I2_IMPORT
#endif /* I2_ICINGA_BUILD */

#endif /* I2ICINGA_H */
