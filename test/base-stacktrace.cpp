// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/stacktrace.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;


/* If you are reading this, you are probably doing so because this test case just failed. This might happen as it
 * heavily depends on platform and compiler behavior. There are two likely causes why this could break:
 *
 *  - Your compiler found new ways to optimize the functions that are called to create a stack, even though we tried
 *    to disable optimizations using #pragmas for some compilers. If you know a way to disable (more) optimizations for
 *    your compiler, you can try if this helps.
 *
 *  - Boost fails to resolve symbol names as we've already seen on some platforms. In this case, you can try again
 *    passing the additional flag `-DICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS=ON` to CMake and see if this helps.
 *
 *  In any case, please report a bug. If you run `make CTEST_OUTPUT_ON_FAILURE=1 test`, the stack trace in question
 *  should be printed. If it looks somewhat meaningful, you can probably ignore a failure of this test case.
 */

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("O0")
#elif defined(__clang__)
#pragma clang optimize off
#elif defined(_MSC_VER)
#pragma optimize("", off)
#endif

BOOST_AUTO_TEST_SUITE(base_stacktrace)

[[gnu::noinline]]
void stack_test_func_b()
{
	boost::stacktrace::stacktrace stack;
	std::ostringstream obuf;
	obuf << StackTraceFormatter(stack);
	std::string result = obuf.str();
	BOOST_CHECK_MESSAGE(!result.empty(), "stack trace must not be empty");
	size_t pos_a = result.find("stack_test_func_a");
	size_t pos_b = result.find("stack_test_func_b");
	BOOST_CHECK_MESSAGE(pos_a != std::string::npos, "'stack_test_func_a' not found\n\n" << result);
	BOOST_CHECK_MESSAGE(pos_b != std::string::npos, "'stack_test_func_b' not found\n\n" << result);
	BOOST_CHECK_MESSAGE(pos_a > pos_b, "'stack_test_func_a' must appear after 'stack_test_func_b'\n\n" << result);
}

[[gnu::noinline]]
void stack_test_func_a()
{
	boost::stacktrace::stacktrace stack;
	std::ostringstream obuf;
	obuf << StackTraceFormatter(stack);
	std::string result = obuf.str();
	BOOST_CHECK_MESSAGE(!result.empty(), "stack trace must not be empty");
	size_t pos_a = result.find("stack_test_func_a");
	BOOST_CHECK_MESSAGE(pos_a != std::string::npos, "'stack_test_func_a' not found\n\n" << result);

	stack_test_func_b();
}

BOOST_AUTO_TEST_CASE(stacktrace)
{
	stack_test_func_a();
}

BOOST_AUTO_TEST_SUITE_END()

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#elif defined(__clang__)
#pragma clang optimize on
#elif defined(_MSC_VER)
#pragma optimize("", on)
#endif
