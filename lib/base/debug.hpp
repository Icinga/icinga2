/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
