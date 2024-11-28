/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "base/io-engine.hpp"
#include "base/utility.hpp"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_io_engine)

BOOST_AUTO_TEST_CASE(timeout_run)
{
	boost::asio::io_context io;
	boost::asio::io_context::strand strand (io);
	int called = 0;

	boost::asio::spawn(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);

		Timeout::Ptr timeout = new Timeout(strand, boost::posix_time::millisec(300), [&called] { ++called; });
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);
	});

	io.run();
	BOOST_CHECK_EQUAL(called, 1);
}

BOOST_AUTO_TEST_CASE(timeout_cancelled)
{
	boost::asio::io_context io;
	boost::asio::io_context::strand strand (io);
	int called = 0;

	boost::asio::spawn(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);
		Timeout::Ptr timeout = new Timeout(strand, boost::posix_time::millisec(300), [&called] { ++called; });

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);

		timeout->Cancel();
		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);
	});

	io.run();
	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(timeout_scope)
{
	boost::asio::io_context io;
	boost::asio::io_context::strand strand (io);
	int called = 0;

	boost::asio::spawn(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);

		{
			Timeout::Ptr timeout = new Timeout(strand, boost::posix_time::millisec(300), [&called] { ++called; });

			timer.expires_from_now(boost::posix_time::millisec(200));
			timer.async_wait(yc);
		}

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(200));
		timer.async_wait(yc);
	});

	io.run();
	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(timeout_due_cancelled)
{
	boost::asio::io_context io;
	boost::asio::io_context::strand strand (io);
	int called = 0;

	boost::asio::spawn(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);
		Timeout::Ptr timeout = new Timeout(strand, boost::posix_time::millisec(300), [&called] { ++called; });

		// Give the timeout enough time to become due while blocking its strand to prevent it from actually running...
		Utility::Sleep(0.4);

		BOOST_CHECK_EQUAL(called, 0);

		// ... so that this shall still work:
		timeout->Cancel();

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(100));
		timer.async_wait(yc);
	});

	io.run();
	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_CASE(timeout_due_scope)
{
	boost::asio::io_context io;
	boost::asio::io_context::strand strand (io);
	int called = 0;

	boost::asio::spawn(strand, [&](boost::asio::yield_context yc) {
		boost::asio::deadline_timer timer (io);

		{
			Timeout::Ptr timeout = new Timeout(strand, boost::posix_time::millisec(300), [&called] { ++called; });

			// Give the timeout enough time to become due while blocking its strand to prevent it from actually running...
			Utility::Sleep(0.4);

			BOOST_CHECK_EQUAL(called, 0);
		} // ... so that Timeout#~Timeout() shall still work here.

		BOOST_CHECK_EQUAL(called, 0);

		timer.expires_from_now(boost::posix_time::millisec(100));
		timer.async_wait(yc);
	});

	io.run();
	BOOST_CHECK_EQUAL(called, 0);
}

BOOST_AUTO_TEST_SUITE_END()
