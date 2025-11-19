/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEBUG_H
#define DEBUG_H

#include "i2-base.hpp"

#ifndef I2_DEBUG
#	define ASSERT(expr) ((void)0)
#else /* I2_DEBUG */
#	define ASSERT(expr) ((expr) ? void(0) : icinga_assert_fail(#expr, __FILE__, __LINE__))
#endif /* I2_DEBUG */

#define VERIFY(expr) ((expr) ? void(0) : icinga_assert_fail(#expr, __FILE__, __LINE__))

#define ABORT(expr) icinga_assert_fail(#expr, __FILE__, __LINE__)

[[noreturn]] inline void icinga_assert_fail(const char *expr, const char *file, int line) noexcept(true)
{
	fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
	std::abort();
}

#endif /* DEBUG_H */
