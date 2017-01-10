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

#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "base/i2-base.hpp"

namespace icinga
{

I2_BASE_API bool InitializeOnceHelper(void (*func)(void), int priority = 0);

#define INITIALIZE_ONCE(func)									\
	namespace { namespace UNIQUE_NAME(io) {							\
		I2_EXPORT bool l_InitializeOnce(icinga::InitializeOnceHelper(func));		\
	} }

#define INITIALIZE_ONCE_WITH_PRIORITY(func, priority)						\
	namespace { namespace UNIQUE_NAME(io) {							\
		I2_EXPORT bool l_InitializeOnce(icinga::InitializeOnceHelper(func, priority));	\
	} }
}

#endif /* INITIALIZE_H */
