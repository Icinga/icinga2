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

#ifndef I2DYN_H
#define I2DYN_H

/**
 * @defgroup dyn Dynamic object library
 *
 * The dynamic object library implements serializable objects which support
 * inheritance.
 */

#include <i2-base.h>

#include <stack>
#include <fstream>

using std::stack;
using std::istream;
using std::ostream;
using std::cin;
using std::endl;

#ifdef I2_DYN_BUILD
#	define I2_DYN_API I2_EXPORT
#else /* I2_DYN_BUILD */
#	define I2_DYN_API I2_IMPORT
#endif /* I2_DYN_BUILD */

#include "debuginfo.h"
#include "expression.h"
#include "expressionlist.h"
#include "configitem.h"
#include "configcompiler.h"

#endif /* I2DYN_H */
