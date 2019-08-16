/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "base/fibers.hpp"
#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <boost_shipped/libs/fiber/examples/asio/round_robin.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

using namespace icinga;
namespace asio = boost::asio;
namespace fibers = boost::fibers;

// Purpose:
// In contrast to fibers::asio::yield this variable is defined in a .cpp file.
thread_local
fibers::asio::yield_t Fibers::Yield;

// ASIO I/O context of the scheduler started by Fibers::StartEngine().
std::shared_ptr<asio::io_context> IoContext;

/* Idea behind the following code:
 *
 * 1) Fibers::StartEngine() fires and forgets N threads, each runs RunWorkerThread().
 * 2) RunWorkerThread() runs an event loop which both handles I/O events and schedules fibers.
 *
 * Unfortunately this isn't that easy...
 *
 * 1) Inside RunWorkerThread() running in thread #1 fibers::asio::round_robin() posts the actual event loop into *IoContext.
 * 2) IoContext->run() picks up that event loop callback and runs it.
 * 3) That callback runs IoContext->run_one() which picks up the next callback and runs it.
 * 4) Inside RunWorkerThread() running in thread #2 fibers::asio::round_robin() posts the actual event loop into *IoContext.
 * 5) Does IoContext->run() running in thread #2 or IoContext->run_one() running in thread #1 pick up that event loop callback?
 *
 * Solution:
 *
 * 1) Fibers::StartEngine() starts thread #1.
 * 2) Inside RunWorkerThread() fibers::asio::round_robin() posts the actual event loop into *IoContext.
 * 3) RunWorkerThread() posts EnterEventLoop() into *IoContext.
 * 4) IoContext->run() picks up the event loop callback (and hopefully not EnterEventLoop()) and runs it.
 * 5) That callback runs IoContext->run_one() which picks up EnterEventLoop() and runs it.
 * 6) EnterEventLoop() notifies Fibers::StartEngine() via l_EventLoopEntry.
 * 7) EnterEventLoop() blocks the event loop via l_EventLoopBarrier so it won't pick up anything, yet.
 * 8) Fibers::StartEngine() starts the next thread the same way until all threads block.
 * 9) Fibers::StartEngine() unblocks all threads via ~unique_lock.
 */

static struct {
	std::mutex Mutex;
	std::condition_variable CV;
} l_EventLoopEntry;

static std::mutex l_EventLoopBarrier;

static void EnterEventLoop()
{
	{
		std::unique_lock<std::mutex> entry (l_EventLoopEntry.Mutex);
		l_EventLoopEntry.CV.notify_one();
	}

	std::unique_lock<std::mutex> barrier (l_EventLoopBarrier);
}

static void RunWorkerThread()
{
	fibers::use_scheduling_algorithm<fibers::asio::round_robin>(IoContext);
	asio::post(*IoContext, &EnterEventLoop);
	IoContext->run();
}

/**
 * Starts Boost.Fiber scheduler with Boost.ASIO support.
 */
void Fibers::StartEngine()
{
	auto concurrency (std::thread::hardware_concurrency());
	IoContext = (std::make_shared<asio::io_context>());

	if (concurrency < 1u) {
		concurrency = 1;
	}

	std::unique_lock<std::mutex> barrier (l_EventLoopBarrier);
	std::unique_lock<std::mutex> entry (l_EventLoopEntry.Mutex);

	for (auto i (concurrency); i; --i) {
		std::thread(&RunWorkerThread).detach();

		l_EventLoopEntry.CV.wait(entry);
	}
}
