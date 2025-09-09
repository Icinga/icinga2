/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <BoostTestTargetConfig.h>
#include <boost/test/test_tools.hpp>
#include <chrono>
#include <thread>

#define ASSERT_CONDITION_WITHIN_TIMEOUT(condition, timeout, level)        \
	/* NOLINTNEXTLINE */                                                  \
	do {                                                                  \
		/* NOLINTNEXTLINE */                                              \
		auto pred = [&, this]() { return static_cast<bool>(condition); }; \
		BOOST_##level(AssertWithTimeout(pred, timeout, #condition));      \
	} while (0)

#define REQUIRE_WITHIN(condition, timeout) ASSERT_CONDITION_WITHIN_TIMEOUT(condition, timeout, REQUIRE)
#define CHECK_WITHIN(condition, timeout) ASSERT_CONDITION_WITHIN_TIMEOUT(condition, timeout, CHECK)
#define WARN_WITHIN(condition, timeout) ASSERT_CONDITION_WITHIN_TIMEOUT(condition, timeout, WARN)

#define ASSERT_CONDITION_EDGE_WITHIN_TIMEOUT(cond, time1, time2, level)    \
	/* NOLINTNEXTLINE */                                                   \
	do {                                                                   \
		/* NOLINTNEXTLINE */                                               \
		auto pred = [&, this]() { return static_cast<bool>(cond); };       \
		BOOST_##level(AssertEdgeWithinTimeout(pred, time1, time2, #cond)); \
	} while (0)

#define REQUIRE_EDGE_WITHIN(cond, time1, time2) ASSERT_CONDITION_EDGE_WITHIN_TIMEOUT(cond, time1, time2, REQUIRE)
#define CHECK_EDGE_WITHIN(cond, time1, time2) ASSERT_CONDITION_EDGE_WITHIN_TIMEOUT(cond, time1, time2, CHECK)
#define WARN_EDGE_WITHIN(cond, time1, time2) ASSERT_CONDITION_EDGE_WITHIN_TIMEOUT(cond, time1, time2, WARN)

#define ASSERT_DONE_WITHIN_TIMEOUT(expr, time1, time2, level)       \
	/* NOLINTNEXTLINE */                                            \
	do {                                                            \
		/* NOLINTNEXTLINE */                                        \
		auto task = [&, this]() { expr; };                          \
		BOOST_##level(AssertDoneWithin(task, time1, time2, #expr)); \
	} while (0)

#define REQUIRE_DONE_BETWEEN(expr, time1, time2) ASSERT_DONE_WITHIN_TIMEOUT(expr, time1, time2, REQUIRE)
#define CHECK_DONE_BETWEEN(expr, time1, time2) ASSERT_DONE_WITHIN_TIMEOUT(expr, time1, time2, CHECK)
#define WARN_DONE_BETWEEN(expr, time1, time2) ASSERT_DONE_WITHIN_TIMEOUT(expr, time1, time2, WARN)

#define REQUIRE_DONE_WITHIN(expr, time1) ASSERT_DONE_WITHIN_TIMEOUT(expr, 0s, time1, REQUIRE)
#define CHECK_DONE_WITHIN(expr, time1) ASSERT_DONE_WITHIN_TIMEOUT(expr, 0s, time1, CHECK)
#define WARN_DONE_WITHIN(expr, time1) ASSERT_DONE_WITHIN_TIMEOUT(expr, 0s, time1, WARN)

namespace icinga {

using namespace std::chrono_literals;

/**
 * Assert that the predicate `fn` will switch from false to true in the given time window
 *
 * @param fn The predicate to check
 * @param timeout The duration in which the predicate is expected to return true
 * @param cond A string representing the condition for use in error messages
 *
 * @return a boost assertion result.
 */
static boost::test_tools::assertion_result AssertWithTimeout(
	const std::function<bool()>& fn,
	const std::chrono::duration<double>& timeout,
	std::string_view cond
)
{
	std::size_t iterations = timeout / 1ms;
	auto stepDur = timeout / iterations;
	for (std::size_t i = 0; i < iterations && !fn(); i++) {
		std::this_thread::sleep_for(stepDur);
	}
	boost::test_tools::assertion_result retVal{fn()};
	retVal.message() << "Condition (" << cond << ") not true within " << timeout.count() << "s";
	return retVal;
}

/**
 * Assert that the predicate `fn` will switch from false to true in the given time window
 *
 * @param fn The predicate to check
 * @param falseUntil The duration for which the predicate is expected to be false
 * @param trueWithin The duration in which the predicate is expected to return true
 * @param cond A string representing the condition for use in error messages
 *
 * @return a boost assertion result.
 */
static boost::test_tools::assertion_result AssertEdgeWithinTimeout(
	const std::function<bool()>& fn,
	const std::chrono::duration<double>& falseUntil,
	const std::chrono::duration<double>& trueWithin,
	std::string_view cond
)
{
	std::size_t iterations = falseUntil / 1ms;
	auto stepDur = falseUntil / iterations;
	for (std::size_t i = 0; i < iterations && !fn(); i++) {
		std::this_thread::sleep_for(stepDur);
	}
	if (fn()) {
		boost::test_tools::assertion_result retVal{false};
		retVal.message() << "Condition (" << cond << ") was true before " << falseUntil.count() << "s";
		return retVal;
	}
	return AssertWithTimeout(fn, trueWithin, cond);
}

/**
 * Assert that the given function takes a duration between lower and upper to complete.
 *
 * @param fn The function to execute
 * @param lower the lower bound to compare the duration against
 * @param upper the upper bound to compare the duration against
 *
 * @return a boost assertion result.
 */
template<class RepStart, class PeriodStart, class RepTimeout, class PeriodTimeout>
static boost::test_tools::assertion_result AssertDoneWithin(
	const std::function<void()>& fn,
	const std::chrono::duration<RepStart, PeriodStart>& lower,
	const std::chrono::duration<RepTimeout, PeriodTimeout>& upper,
	std::string_view fnString
)
{
	auto start = std::chrono::steady_clock::now();
	fn();
	auto duration = std::chrono::steady_clock::now() - start;
	boost::test_tools::assertion_result retVal{duration > lower && duration < upper};
	retVal.message() << fnString << " took " << std::chrono::duration<double>(duration).count() << "s";
	return retVal;
}

} // namespace icinga
