/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "base/atomic.hpp"
#include "base/defer.hpp"
#include "base/fibers.hpp"
#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <BoostTestTargetConfig.h>
#include <cstddef>
#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#endif /* _WIN32 */

using namespace icinga;

#ifndef _WIN32
static Atomic<bool> l_StartedEngine (false);
static const char l_TestData[] = R"EOF(
* This text shall be written to a pipe end w/o errors
* The amount of bytes written shall match the text's length
* The pipe end shall be closed w/o errors
* The written data shall be read from the other pipe end w/o errors
* The amount of bytes read shall match the amount of bytes written
* The other pipe end shall be closed w/o errors
* The read data shall match this text
)EOF";
#endif /* _WIN32 */

BOOST_AUTO_TEST_SUITE(base_fibers)

BOOST_AUTO_TEST_CASE(working)
{
#ifndef _WIN32
	namespace asio = boost::asio;
	namespace fibers = boost::fibers;
	namespace sys = boost::system;

	if (!l_StartedEngine.exchange(true)) {
		Fibers::StartEngine(1);
	}

	int fildes[2];
	BOOST_CHECK(pipe(fildes) == 0);

	struct PerPipeEnd {
		asio::posix::stream_descriptor PipeEnd;
		struct {
			sys::error_code Io, Close;
		} Errors;
		size_t BytesTransferred;
		Atomic<bool> Done;

		inline PerPipeEnd(int fd) : PipeEnd(*Fibers::Io, fd), BytesTransferred(0), Done(false)
		{
		}
	};

	PerPipeEnd r (fildes[0]), w (fildes[1]);

	bool equal = false;

	fibers::fiber([&r, &equal]() {
		Defer signalDone ([&r]() { r.Done.store(true); });

		char buf[4096] = {};

		try {
			r.BytesTransferred = asio::async_read(r.PipeEnd, asio::mutable_buffer(buf, sizeof(buf) / sizeof(buf[0])), yf);
		} catch (const sys::system_error& err) {
			r.Errors.Io = err.code();
			return;
		}

		r.PipeEnd.close(r.Errors.Close);

		buf[sizeof(buf) / sizeof(buf[0]) - 1u] = 0;
		equal = !strcmp(buf, l_TestData);
	}).detach();

	fibers::fiber([&w]() {
		Defer signalDone ([&w]() { w.Done.store(true); });

		try {
			w.BytesTransferred = asio::async_write(w.PipeEnd, asio::const_buffer(l_TestData, sizeof(l_TestData) / sizeof(l_TestData[0])), yf);
		} catch (const sys::system_error& err) {
			w.Errors.Io = err.code();
			return;
		}

		w.PipeEnd.close(w.Errors.Close);
	}).detach();

	while (!(r.Done.load() && w.Done.load())) {
	}

	BOOST_CHECK(!w.Errors.Io);
	BOOST_CHECK(w.BytesTransferred == sizeof(l_TestData) / sizeof(l_TestData[0]));
	BOOST_CHECK(!w.Errors.Close);

	BOOST_CHECK(!r.Errors.Io);
	BOOST_CHECK(r.BytesTransferred == w.BytesTransferred);
	BOOST_CHECK(!r.Errors.Close);

	BOOST_CHECK(equal);
#endif /* _WIN32 */
}

BOOST_AUTO_TEST_SUITE_END()
