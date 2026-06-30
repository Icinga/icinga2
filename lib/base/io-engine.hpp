// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IO_ENGINE_H
#define IO_ENGINE_H

#include "base/atomic.hpp"
#include "base/debug.hpp"
#include "base/exception.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include "base/shared.hpp"
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <stdexcept>
#include <boost/context/fixedsize_stack.hpp>
#include <boost/exception/all.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#if BOOST_VERSION >= 108700
#	include <boost/asio/detached.hpp>
#endif // BOOST_VERSION >= 108700

namespace icinga
{

/**
 * Scope lock for CPU-bound work done in an I/O thread
 *
 * @ingroup base
 */
class CpuBoundWork
{
public:
	CpuBoundWork(boost::asio::yield_context yc, boost::asio::io_context::strand&);
	CpuBoundWork(const CpuBoundWork&) = delete;
	CpuBoundWork(CpuBoundWork&&) = delete;
	CpuBoundWork& operator=(const CpuBoundWork&) = delete;
	CpuBoundWork& operator=(CpuBoundWork&&) = delete;

	inline ~CpuBoundWork()
	{
		Done();
	}

	void Done();

private:
	static bool TryAcquireSlot();

	bool m_Done;
};

/**
 * Condition variable which doesn't block I/O threads
 *
 * @ingroup base
 */
class AsioConditionVariable
{
public:
	AsioConditionVariable(boost::asio::io_context& io);

	void Wait(boost::asio::yield_context yc);
	bool NotifyOne();
	size_t NotifyAll();

private:
	boost::asio::steady_timer m_Timer;
};

/**
 * Async I/O engine
 *
 * @ingroup base
 */
class IoEngine
{
	friend CpuBoundWork;

public:
	IoEngine(const IoEngine&) = delete;
	IoEngine(IoEngine&&) = delete;
	IoEngine& operator=(const IoEngine&) = delete;
	IoEngine& operator=(IoEngine&&) = delete;
	~IoEngine();

	static IoEngine& Get();

	/**
	 * Checks whether the given strand is currently running in the calling thread.
	 *
	 * This is a simple wrapper around @c running_in_this_thread() with a little but significant difference:
	 * It is marked as @c noinline to prevent the compiler from ever inlining the call to this function and
	 * thus potentially optimizing away the thread-local storage access that is required for this function
	 * to work correctly. This is especially important for the case where the caller is a coroutine that have
	 * some suspension points between the calls to this function, and cause the compiler to assume that the
	 * thread-local access performed by @c running_in_this_thread() is invariant across these suspensions and
	 * thus optimize it by caching the result in a register or on the stack, which would lead to incorrect
	 * results after resuming the coroutine on a different thread. For more details, see [^1][^2][^3].
	 *
	 * [^1]: https://github.com/chriskohlhoff/asio/issues/1366
	 * [^2]: https://bugs.llvm.org/show_bug.cgi?id=19177
	 * [^3]: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=26461
	 */
	BOOST_NOINLINE static bool IsStrandRunningOnThisThread(const boost::asio::io_context::strand& strand)
	{
		return strand.running_in_this_thread();
	}

	boost::asio::io_context& GetIoContext();

	static inline size_t GetCoroutineStackSize() {
#ifdef _WIN32
		// Increase the stack size for Windows coroutines to prevent exception corruption.
		// Rationale: Low cost Windows agent only & https://github.com/Icinga/icinga2/issues/7431
		return 8 * 1024 * 1024;
#else /* _WIN32 */
		// Increase the stack size for Linux/Unix coroutines for many JSON objects on the stack.
		// This may help mitigate possible stack overflows. https://github.com/Icinga/icinga2/issues/7532
		return 256 * 1024;
		//return boost::coroutines::stack_allocator::traits_type::default_size(); // Default 64 KB
#endif /* _WIN32 */
	}

	template <typename Handler, typename Function>
	static void SpawnCoroutine(Handler& h, Function f) {
		auto wrapper = [f = std::move(f)](boost::asio::yield_context yc) {
			try {
				f(yc);
			} catch (const std::exception& ex) {
				Log(LogCritical, "IoEngine") << "Exception in coroutine: " << DiagnosticInformation(ex);
			} catch (...) {
				try {
					Log(LogCritical, "IoEngine", "Exception in coroutine!");
				} catch (...) {
				}

				// Required for proper stack unwinding when coroutines are destroyed.
				// https://github.com/boostorg/coroutine/issues/39
				throw;
			}
		};

#if BOOST_VERSION >= 108700
		boost::asio::spawn(h,
			std::allocator_arg, boost::context::fixedsize_stack(GetCoroutineStackSize()),
			std::move(wrapper),
			boost::asio::detached
		);
#else // BOOST_VERSION >= 108700
		boost::asio::spawn(h, std::move(wrapper), boost::coroutines::attributes(GetCoroutineStackSize()));
#endif // BOOST_VERSION >= 108700
	}

private:
	IoEngine();

	void RunEventLoop();

	static LazyInit<std::unique_ptr<IoEngine>> m_Instance;

	boost::asio::io_context m_IoContext;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_KeepAlive;
	std::vector<std::thread> m_Threads;

	std::atomic_uint_fast32_t m_CpuBoundSemaphore;
	std::mutex m_CpuBoundWaitingMutex;
	std::vector<std::pair<boost::asio::io_context::strand, Shared<AsioConditionVariable>::Ptr>> m_CpuBoundWaiting;
};

class TerminateIoThread : public std::exception
{
};

/**
 * Awaitable flag which doesn't block I/O threads, inspired by threading.Event from Python
 *
 * @ingroup base
 */
class AsioEvent
{
public:
	AsioEvent(boost::asio::io_context& io, bool init = false);

	void Set();
	void Clear();
	void Wait(boost::asio::yield_context yc);

private:
	boost::asio::deadline_timer m_Timer;
};

/**
 * Like AsioEvent, which only allows waiting for an event to be set, but additionally supports waiting for clearing
 *
 * @ingroup base
 */
class AsioDualEvent
{
public:
	AsioDualEvent(boost::asio::io_context& io, bool init = false);

	void Set();
	void Clear();

	void WaitForSet(boost::asio::yield_context yc);
	void WaitForClear(boost::asio::yield_context yc);

private:
	AsioEvent m_IsTrue, m_IsFalse;
};

/**
 * I/O timeout emulator
 *
 * This class provides a workaround for Boost.ASIO's lack of built-in timeout support.
 * While Boost.ASIO handles asynchronous operations, it does not natively support timeouts for these operations.
 * This class uses a boost::asio::deadline_timer to emulate a timeout by scheduling a callback to be triggered
 * after a specified duration, effectively adding timeout behavior where none exists.
 * The callback is executed within the provided strand, ensuring thread-safety.
 *
 * The constructor returns immediately after scheduling the timeout callback.
 * The callback itself is invoked asynchronously when the timeout occurs.
 * This allows the caller to continue execution while the timeout is running in the background.
 *
 * The class provides a Cancel() method to unschedule any pending callback. If the callback has already been run,
 * calling Cancel() has no effect. This method can be used to abort the timeout early if the monitored operation
 * completes before the callback has been run. The Timeout destructor also automatically cancels any pending callback.
 * A callback is considered pending even if the timeout has already expired,
 * but the callback has not been executed yet due to a busy strand.
 *
 * @ingroup base
 */
class Timeout
{
public:
	using Timer = boost::asio::deadline_timer;

	/**
	 * Schedules onTimeout to be triggered after timeoutFromNow on strand.
	 *
	 * @param strand The strand in which the callback will be executed.
	 *				 The caller must also run in this strand, as well as Cancel() and the destructor!
	 * @param timeoutFromNow The duration after which the timeout callback will be triggered.
	 * @param onTimeout The callback to invoke when the timeout occurs.
	 */
	template<class OnTimeout>
	Timeout(boost::asio::io_context::strand& strand, const Timer::duration_type& timeoutFromNow, OnTimeout onTimeout)
		: m_Timer(strand.context(), timeoutFromNow), m_Cancelled(Shared<Atomic<bool>>::Make(false))
	{
		ASSERT(IoEngine::IsStrandRunningOnThisThread(strand));

		m_Timer.async_wait(boost::asio::bind_executor(
			strand, [cancelled = m_Cancelled, onTimeout = std::move(onTimeout)](boost::system::error_code ec) {
				if (!ec && !cancelled->load()) {
					onTimeout();
				}
			}
		));
	}

	Timeout(const Timeout&) = delete;
	Timeout(Timeout&&) = delete;
	Timeout& operator=(const Timeout&) = delete;
	Timeout& operator=(Timeout&&) = delete;

	/**
	 * Cancels any pending timeout callback.
	 *
	 * Must be called in the strand in which the callback was scheduled!
	 */
	~Timeout()
	{
		Cancel();
	}

	void Cancel();

private:
	Timer m_Timer;

	/**
	 * Indicates whether the Timeout has been cancelled.
	 *
	 * This must be Shared<> between the lambda in the constructor and Cancel() for the case
	 * the destructor calls Cancel() while the lambda is already queued in the strand.
	 * The whole Timeout instance can't be kept alive by the lambda because this would delay the destructor.
	 */
	Shared<Atomic<bool>>::Ptr m_Cancelled;
};

}

#endif /* IO_ENGINE_H */
