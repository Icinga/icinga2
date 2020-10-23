/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/stacktrace.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;


#pragma GCC push_options
#pragma GCC optimize ("O0")
#pragma clang optimize off

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
	stack_test_func_b();
}

BOOST_AUTO_TEST_CASE(stacktrace)
{
	stack_test_func_a();
}

BOOST_AUTO_TEST_SUITE_END()

#pragma GCC pop_options
#pragma clang optimize on
