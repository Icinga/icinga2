/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include <thread>
#include <utility>
#include <vector>
#include <stdexcept>
#include <boost/context/fixedsize_stack.hpp>
#include <boost/exception/all.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

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
	CpuBoundWork(boost::asio::yield_context yc);
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
	bool m_Done;
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

	static inline
	void YieldCurrentCoroutine(boost::asio::yield_context yc)
	{
		Get().m_AlreadyExpiredTimer.async_wait(yc);
	}

private:
	IoEngine();

	void RunEventLoop();

	static LazyInit<std::unique_ptr<IoEngine>> m_Instance;

	boost::asio::io_context m_IoContext;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_KeepAlive;
	std::vector<std::thread> m_Threads;
	boost::asio::deadline_timer m_AlreadyExpiredTimer;
	std::atomic_int_fast32_t m_CpuBoundSemaphore;
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
		VERIFY(strand.running_in_this_thread());

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
