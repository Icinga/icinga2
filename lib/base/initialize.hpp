/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "base/i2-base.hpp"

namespace icinga
{

#define I2_TOKENPASTE(x, y) x ## y
#define I2_TOKENPASTE2(x, y) I2_TOKENPASTE(x, y)

#define I2_UNIQUE_NAME(prefix) I2_TOKENPASTE2(prefix, __COUNTER__)

bool InitializeOnceHelper(void (*func)(), int priority = 0);

#define INITIALIZE_ONCE(func)									\
	namespace { namespace I2_UNIQUE_NAME(io) {							\
		bool l_InitializeOnce(icinga::InitializeOnceHelper(func));		\
	} }

#define INITIALIZE_ONCE_WITH_PRIORITY(func, priority)						\
	namespace { namespace I2_UNIQUE_NAME(io) {							\
		bool l_InitializeOnce(icinga::InitializeOnceHelper(func, priority));	\
	} }
}

#endif /* INITIALIZE_H */
