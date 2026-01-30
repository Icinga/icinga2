/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <functional>
#include <future>
#include <thread>
#include <utility>

#define REQUIRE_JOINS_WITHIN(t, timeout)                                                \
	BOOST_REQUIRE_MESSAGE(t.TryJoinFor(timeout), "Thread not joinable within timeout.")
#define CHECK_JOINS_WITHIN(t, timeout)                                                  \
	BOOST_REQUIRE_MESSAGE(t.TryJoinFor(timeout), "Thread not joinable within timeout.")
#define TEST_JOINS_WITHIN(t, timeout)                                                   \
	BOOST_REQUIRE_MESSAGE(t.TryJoinFor(timeout), "Thread not joinable within timeout.")

#define REQUIRE_JOINABLE(t) BOOST_REQUIRE_MESSAGE(t.Joinable(), "Thread not joinable.")
#define CHECK_JOINABLE(t) BOOST_REQUIRE_MESSAGE(t.Joinable(), "Thread not joinable.")
#define TEST_JOINABLE(t) BOOST_REQUIRE_MESSAGE(t.Joinable(), "Thread not joinable.")


namespace icinga {

class TestThread
{
public:
	explicit TestThread(std::function<void()> fn) : TestThread(std::move(fn), std::promise<void>{}) {}

	bool Joinable()
	{
		auto status = m_JoinFuture.wait_for(std::chrono::milliseconds{0});
		return status == std::future_status::ready;
	}

	template<class Rep, class Period>
	bool TryJoinFor(std::chrono::duration<Rep, Period> timeout)
	{
		auto status = m_JoinFuture.wait_for(timeout);
		if (status == std::future_status::ready) {
			m_Thread.join();
			return true;
		}
		return false;
	}

	bool TryJoin() { return TryJoinFor(std::chrono::milliseconds{0}); }

private:
	explicit TestThread(std::function<void()> fn, std::promise<void> joinPromise)
		: m_JoinFuture(joinPromise.get_future()), m_Thread([fn = std::move(fn), jp = std::move(joinPromise)]() mutable {
			  fn();
			  jp.set_value();
		  })
	{
	}

	std::future<void> m_JoinFuture;
	std::thread m_Thread;
};

} // namespace icinga
