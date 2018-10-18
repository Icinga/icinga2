/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "i2-base.hpp"

#ifndef I2_DEBUG
#	define ASSERT(expr) ((void)0)
#else /* I2_DEBUG */
#	define ASSERT(expr) ((expr) ? 0 : icinga_assert_fail(#expr, __FILE__, __LINE__))
#endif /* I2_DEBUG */

#define VERIFY(expr) ((expr) ? 0 : icinga_assert_fail(#expr, __FILE__, __LINE__))

#ifdef _MSC_VER
#	define NORETURNPRE __declspec(noreturn)
#else /* _MSC_VER */
#	define NORETURNPRE
#endif /* _MSC_VER */

#ifdef __GNUC__
#	define NORETURNPOST __attribute__((noreturn))
#else /* __GNUC__ */
#	define NORETURNPOST
#endif /* __GNUC__ */

NORETURNPRE int icinga_assert_fail(const char *expr, const char *file, int line) NORETURNPOST;

#ifdef _MSC_VER
#	pragma warning( push )
#	pragma warning( disable : 4646 ) /* function declared with __declspec(noreturn) has non-void return type */
#endif /* _MSC_VER */

inline int icinga_assert_fail(const char *expr, const char *file, int line)
{
	fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
	std::abort();

#if !defined(__GNUC__) && !defined(_MSC_VER)
	return 0;
#endif /* !defined(__GNUC__) && !defined(_MSC_VER) */
}

#ifdef _MSC_VER
#	pragma warning( pop )
#endif /* _MSC_VER */

#endif /* DEBUG_H */
