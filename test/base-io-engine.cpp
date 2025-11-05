/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "base/io-engine.hpp"
#include "base/utility.hpp"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <BoostTestTargetConfig.h>
#include <thread>
#include "base/io-future.hpp"

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_io_engine)

BOOST_AUTO_TEST_CASE(timeout_run)
{
	boost::asio::io_context io;
	boost::asio::io_context::strand strand (io);
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);

		Timeout timeout (strand, boost::posix_time::millisec(300), [&called] { ++called; });
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
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
	boost::asio::io_context::strand strand (io);
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);
		Timeout timeout (strand, boost::posix_time::millisec(300), [&called] { ++called; });

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);

		timeout.Cancel();
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
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
	boost::asio::io_context::strand strand (io);
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);

		{
			Timeout timeout (strand, boost::posix_time::millisec(300), [&called] { ++called; });

			timer.expires_from_now(boost::posix_time::millisec(200));
			timer.async_wait(yc);
		}

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
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
	boost::asio::io_context::strand strand (io);
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);
		Timeout timeout (strand, boost::posix_time::millisec(300), [&called] { ++called; });

		// Give the timeout enough time to become due while blocking its strand to prevent it from actually running...
		Utility::Sleep(0.4);

		BOOST_CHECK_EQUAL(called, 0);

		// ... so that this shall still work:
		timeout.Cancel();

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(100));
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
	boost::asio::io_context::strand strand (io);
	int called = 0;

	IoEngine::SpawnCoroutine(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);

		{
			Timeout timeout (strand, boost::posix_time::millisec(300), [&called] { ++called; });

			// Give the timeout enough time to become due while blocking its strand to prevent it from actually running...
			Utility::Sleep(0.4);

			BOOST_CHECK_EQUAL(called, 0);
		} // ... so that Timeout#~Timeout() shall still work here.

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(100));
		timer.async_wait(yc);
	});

	std::thread eventLoop ([&io] { io.run(); });
	io.run();
	eventLoop.join();

	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(future_early_value)
{
	boost::asio::io_context io;

	AsioPromise<bool> promise;
	promise.SetValue(true);

	IoEngine::SpawnCoroutine(io, [&, future = promise.GetFuture()](boost::asio::yield_context yc) {
		auto val = future->Get(yc);
		BOOST_REQUIRE(val);
	});

	io.run();
}

BOOST_AUTO_TEST_CASE(future_value)
{
	boost::asio::io_context io;

	AsioPromise<bool> promise;
	std::atomic_bool before = false;
	std::atomic_bool after = false;

	IoEngine::SpawnCoroutine(io, [&, future = promise.GetFuture()](boost::asio::yield_context yc) {
		before = true;
		auto val = future->Get(yc);
		BOOST_REQUIRE(val);
		after = true;
	});

	std::thread testThread
	{
		[&io]() {
			io.run();
		}};

	while (!before) {
		Utility::Sleep(0.01);
	}

	BOOST_REQUIRE(before);
	BOOST_REQUIRE(!after);

	promise.SetValue(true);

	while (!after) {
		Utility::Sleep(0.01);
	}
	
	BOOST_REQUIRE(before);
	BOOST_REQUIRE(after);

	testThread.join();
}

BOOST_AUTO_TEST_CASE(future_early_exception)
{
	boost::asio::io_context io;

	AsioPromise<bool> promise;
	promise.SetException(std::runtime_error{"test"});

	IoEngine::SpawnCoroutine(io, [&, future = promise.GetFuture()](boost::asio::yield_context yc) {
		BOOST_REQUIRE_EXCEPTION(future->Get(yc), std::runtime_error, [](const std::exception& ex) -> bool {
			return std::string_view{ex.what()} == "test";
		});
	});

	io.run();
}

BOOST_AUTO_TEST_CASE(future_exception)
{
	boost::asio::io_context io;

	AsioPromise<bool> promise;
	std::atomic_bool before = false;
	std::atomic_bool after = false;

	IoEngine::SpawnCoroutine(io, [&, future = promise.GetFuture()](boost::asio::yield_context yc) {
		before = true;
		BOOST_REQUIRE_EXCEPTION(future->Get(yc), std::runtime_error, [](const std::exception& ex) -> bool {
			return std::string_view{ex.what()} == "test";
		});
		after = true;
	});

	std::thread testThread
	{
		[&io]() {
			io.run();
		}};

	while (!before) {
		Utility::Sleep(0.01);
	}

	BOOST_REQUIRE(before);
	BOOST_REQUIRE(!after);

	promise.SetException(std::runtime_error{"test"});

	while (!after) {
		Utility::Sleep(0.01);
	}
	
	BOOST_REQUIRE(before);
	BOOST_REQUIRE(after);

	testThread.join();
}

BOOST_AUTO_TEST_SUITE_END()
