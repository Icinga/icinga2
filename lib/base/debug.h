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

#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#include <stdio.h>

#ifdef NDEBUG
#	if defined(__clang__) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#		define ASSERT(expr) __builtin_unreachable()
#	else
#		define ASSERT(expr) ((void)0)
#	endif
#else /* NDEBUG */
#	define ASSERT(expr) ((expr) ? 0 : icinga_assert_fail(#expr, __FILE__, __LINE__))
#endif /* NDEBUG */

#define VERIFY(expr) ((expr) ? 0 : icinga_assert_fail(#expr, __FILE__, __LINE__))

int icinga_assert_fail(const char *expr, const char *file, int line) __attribute__((noreturn));

inline int icinga_assert_fail(const char *expr, const char *file, int line)
{
	fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
	abort();
}

#endif /* DEBUG_H */
