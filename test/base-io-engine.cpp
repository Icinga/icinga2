// SPDX-FileCopyrightText: 2024 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/io-engine.hpp"
#include "base/utility.hpp"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <BoostTestTargetConfig.h>
#include <thread>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_io_engine)

BOOST_AUTO_TEST_CASE(timeout_run)
{
	boost::asio::io_context io;
	IoStrand strand (io.get_executor());
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::steady_timer timer (io);

		Timeout timeout (strand, 300ms, [&called] { ++called; });
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_after(200ms);
		timer.async_wait(yc);
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_after(200ms);
		timer.async_wait(yc);
	});

	std::thread eventLoop ([&io] { io.run(); });
	io.run();
	eventLoop.join();

	BOOST_CHECK_EQUAL(called, 1);
}

BOOST_AUTO_TEST_CASE(timeout_cancelled)
{
	boost::asio::io_context io;
	IoStrand strand (io.get_executor());
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::steady_timer timer (io);
		Timeout timeout (strand, 300ms, [&called] { ++called; });

		timer.expires_after(200ms);
		timer.async_wait(yc);

		timeout.Cancel();
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_after(200ms);
		timer.async_wait(yc);
	});

	std::thread eventLoop ([&io] { io.run(); });
	io.run();
	eventLoop.join();

	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(timeout_scope)
{
	boost::asio::io_context io;
	IoStrand strand (io.get_executor());
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::steady_timer timer (io);

		{
			Timeout timeout (strand, 300ms, [&called] { ++called; });

			timer.expires_after(200ms);
			timer.async_wait(yc);
		}

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_after(200ms);
		timer.async_wait(yc);
	});

	std::thread eventLoop ([&io] { io.run(); });
	io.run();
	eventLoop.join();

	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(timeout_due_cancelled)
{
	boost::asio::io_context io;
	IoStrand strand (io.get_executor());
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::steady_timer timer (io);
		Timeout timeout (strand, 300ms, [&called] { ++called; });

		// Give the timeout enough time to become due while blocking its strand to prevent it from actually running...
		Utility::Sleep(0.4);

		BOOST_CHECK_EQUAL(called, 0);

		// ... so that this shall still work:
		timeout.Cancel();

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_after(100ms);
		timer.async_wait(yc);
	});

	std::thread eventLoop ([&io] { io.run(); });
	io.run();
	eventLoop.join();

	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(timeout_due_scope)
{
	boost::asio::io_context io;
	IoStrand strand (io.get_executor());
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::steady_timer timer (io);

		{
			Timeout timeout (strand, 300ms, [&called] { ++called; });

			// Give the timeout enough time to become due while blocking its strand to prevent it from actually running...
			Utility::Sleep(0.4);

			BOOST_CHECK_EQUAL(called, 0);
		} // ... so that Timeout#~Timeout() shall still work here.

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_after(100ms);
		timer.async_wait(yc);
	});

	std::thread eventLoop ([&io] { io.run(); });
	io.run();
	eventLoop.join();

	BOOST_CHECK_EQUAL(called, 0);
}

// Test case for issue https://github.com/boostorg/context/issues/323
// Affected boost versions are 1.88 - 1.91.
// This can be removed once these versions are no longer supported.
// Note that the fix for this doesn't strictly live in io-engine, but in a header that is
// force-included globally via CMake.
BOOST_AUTO_TEST_CASE(resume_on_other_thread_keeps_exception_state)
{
	boost::asio::io_context io;

	// A coroutine that suspends immediately and sets a bool when it has resumed and finished.
	bool coroutineFinished{};
	IoEngine::SpawnCoroutine(io, [&](boost::asio::yield_context yc) {
		boost::asio::post(io, yc);
		coroutineFinished = true;
	});

	// Run the coroutine to the first suspension point inside this thread.
	BOOST_REQUIRE_EQUAL(io.run_one(), 1u);

	try {
		// Set the exception state to a valid exception.
		throw std::runtime_error("test");
	} catch (const std::exception&) {
		// Verify that the exception state is valid.
		BOOST_REQUIRE(std::current_exception());

		// Now if the coroutine stack holds a cached result of __cxa_get_globals(), resuming
		// the coroutine in a different thread will cause it to be overwritten.
		std::thread other([&io] { io.run_one(); });
		other.join();
		BOOST_REQUIRE(coroutineFinished);

		// Check if the exception state has been overwritten in the thread above.
		BOOST_REQUIRE(std::current_exception());
	}
}

BOOST_AUTO_TEST_SUITE_END()
