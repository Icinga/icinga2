/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef DEBUG_H
#define DEBUG_H

#include "i2-base.h"

#ifdef NDEBUG
#	define ASSERT(expr) ((void)0)
#else /* NDEBUG */
#	define ASSERT(expr) ((expr) ? 0 : icinga_assert_fail(#expr, __FILE__, __LINE__))
#endif /* NDEBUG */

#define VERIFY(expr) ((expr) ? 0 : icinga_assert_fail(#expr, __FILE__, __LINE__))

#ifdef __GNUC__
#	define NORETURN __attribute__((noreturn))
#else /* __GNUC__ */
#	define NORETURN
#endif /* __GNUC__ */

int icinga_assert_fail(const char *expr, const char *file, int line) NORETURN;

inline int icinga_assert_fail(const char *expr, const char *file, int line)
{
	fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
	abort();

#ifndef __GNUC__
	return 0;
#endif /* __GNUC__ */
}

#endif /* DEBUG_H */
