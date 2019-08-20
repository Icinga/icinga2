/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "base/fibers.hpp"
#include <boost/thread/barrier.hpp>
#include <boost/fiber/all.hpp>
#include <memory>
#include <thread>

using namespace icinga;
using namespace Fibers;
namespace asio = boost::asio;
namespace fibers = boost::fibers;

namespace icinga
{

namespace Fibers
{

// ASIO I/O context of the scheduler started by StartEngine().
std::unique_ptr<asio::io_context> Io;

static inline
void RunWorker(uint32_t threads, boost::barrier& barrier)
{
	fibers::use_scheduling_algorithm<fibers::algo::work_stealing>(threads);
	barrier.wait();

	fibers::mutex mutex;
	fibers::condition_variable_any cv;

	mutex.lock();
	cv.wait(mutex);
	mutex.unlock();
}

/**
 * Starts Boost.Fiber scheduler with Boost.ASIO support.
 *
 * @param threads How many threads to start.
 */
void StartEngine(uint32_t threads)
{
	Io.reset(new asio::io_context());

	if (threads < 1u) {
		threads = 1;
	}

	boost::barrier barrier (threads + 1u);
	auto scheduler ([threads, &barrier]() { RunWorker(threads, barrier); });

	for (auto i (threads); i; --i) {
		std::thread(scheduler).detach();
	}

	barrier.wait();
}

} /* Fibers */

// Just for convenience, "yf" is much less to type than "Fibers::Yield()".
Fibers::Yield yf;

} /* icinga */
