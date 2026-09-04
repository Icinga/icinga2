// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IO_ENGINE_H
#define IO_ENGINE_H

#include "base/atomic.hpp"
#include "base/debug.hpp"
#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include "base/shared.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <stdexcept>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <boost/exception/all.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#if BOOST_VERSION >= 108700
#      include <boost/asio/detached.hpp>
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

class IoEngine;

/**
 * The result (or exception) of a coroutine, handed over to a thread waiting for it.
 *
 * This is used instead of a `std::promise`/`std::future` because those use thread local storage and if enough of the
 * coroutine function can be inlined further optimizations may lead to crashes when the TLS is initialized by one thread
 * before coroutine suspension and the next thread tries to use it, which can lead to very hard to diagnose issues.
 */
template<class T>
class SyncResult {
	friend IoEngine;

	using ValueType = std::variant<std::monostate, std::conditional_t<std::is_void_v<T>, bool, T>, std::exception_ptr>;

public:
	/**
	 * Thrown by `Get()` if the coroutine was destroyed before producing anything.
	 *
	 * This is the equivalent of `std::future_error(std::future_errc::broken_promise)` and is thrown when the io_context
	 * is destroyed while the coroutine is still suspended or has never been started.
	 */
	struct CoroutineAbandoned : std::exception {
		[[nodiscard]] const char* what() const noexcept override
		{
			return "Coroutine was destroyed before producing a result";
		}
	};

	/**
	 * Blocks until the coroutine finished, then returns its result or re-throws its exception.
	 *
	 * Never call this from an I/O thread, and especially not from the very strand the coroutine runs in -- that blocks
	 * the thread which is supposed to complete the coroutine. The result is moved out, so call this at most once.
	 */
	T Get()
	{
		std::unique_lock lock{m_Mutex};
		m_Cv.wait(lock, [this] { return !std::holds_alternative<std::monostate>(m_Value); });

		if (std::holds_alternative<std::exception_ptr>(m_Value)) {
			std::rethrow_exception(std::get<std::exception_ptr>(m_Value));
		}

		if constexpr (std::is_void_v<T>) {
			return;
		} else {
			return std::move(std::get<T>(m_Value));
		}
	}

	/**
	 * Waits up to timeout for the coroutine to finish, without consuming the result.
	 *
	 * @return Whether the result is ready, i.e. whether Get() would return without blocking.
	 */
	template<class Rep, class Period>
	bool WaitFor(const std::chrono::duration<Rep, Period>& timeout)
	{
		std::unique_lock lock(m_Mutex);
		return m_Cv.wait_for(lock, timeout, [this] { return !std::holds_alternative<std::monostate>(m_Value); });
	}

private:
	/**
	 * Stores the coroutine's return value and wakes up a waiting Get().
	 */
	template<class U, class V = T, class = std::enable_if_t<!std::is_void_v<V>>>
	void SetValue(U&& v)
	{
		std::scoped_lock lock(m_Mutex);
		m_Value = std::forward<U>(v);
		m_Cv.notify_all();
	}

	/**
	 * Signals completion of a void returning coroutine and wakes up a waiting Get().
	 */
	template<class V = T, class = std::enable_if_t<std::is_void_v<V>>>
	void SetValue()
	{
		std::scoped_lock lock(m_Mutex);
		m_Value = true;
		m_Cv.notify_all();
	}

	/**
	 * Stores an exception to be re-thrown by Get() and wakes it up.
	 */
	void SetException(std::exception_ptr ep)
	{
		std::scoped_lock lock(m_Mutex);
		m_Value = ValueType{ep};
		m_Cv.notify_all();
	}

	/**
	 * Reports that the coroutine will never produce a result, unless it already did.
	 *
	 * Called by SpawnSyncCoroutine() once the coroutine body is destroyed, so that a Get() waiting for a coroutine
	 * which never ran to completion fails instead of blocking forever.
	 */
	void SetAbandoned()
	{
		std::scoped_lock lock(m_Mutex);

		if (std::holds_alternative<std::monostate>(m_Value)) {
			m_Value = ValueType{std::make_exception_ptr(CoroutineAbandoned{})};
			m_Cv.notify_all();
		}
	}

	std::mutex m_Mutex;
	std::condition_variable m_Cv;
	ValueType m_Value;
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

	/**
	 * Spawns a stackful coroutine running f, without caring about its outcome.
	 *
	 * Exceptions escaping f are logged and swallowed. To get f's result or exception instead,
	 * use SpawnSyncCoroutine().
	 *
	 * @param h The execution context, executor or strand to run the coroutine in.
	 * @param f The coroutine body, taking a boost::asio::yield_context.
	 */
	template<class Handler, class Function>
	static void SpawnCoroutine(Handler& h, Function f)
	{
		static_assert(std::is_void_v<CoroutineResultType<Function>>,
			"A detached coroutine's return value would be silently discarded. "
			"Either make f return void or use SpawnSyncCoroutine().");

		SpawnCoroutineImpl(h, [f = std::move(f)](boost::asio::yield_context yc) mutable {
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
		});
	}

	/**
	 * Spawns a stackful coroutine running f and returns a handle to its eventual result.
	 *
	 * Exceptions escaping f are stored in the returned handle and rethrown on `handle->Get()`.
	 *
	 * @param h The execution context, executor or strand to run the coroutine in.
	 * @param f The coroutine body, taking a boost::asio::yield_context.
	 *
	 * @return Shared handle to f's result, which stays valid regardless of who drops it first.
	 */
	template<class Handler, class Function>
	static auto SpawnSyncCoroutine(Handler& h, Function&& f)
	{
		using RetType = CoroutineResultType<Function>;

		auto result = std::make_shared<SyncResult<RetType>>();

		struct AbandonOnDrop {
			std::shared_ptr<SyncResult<RetType>> r;
			~AbandonOnDrop()
			{
				if (r) {
					r->SetAbandoned();
				}
			}
			explicit AbandonOnDrop(std::shared_ptr<SyncResult<RetType>> r) : r{std::move(r)} {};
			AbandonOnDrop(AbandonOnDrop&&) = default;
			AbandonOnDrop(const AbandonOnDrop&) = delete;
		};
		SpawnCoroutineImpl(
			h,
			[result,
			 f = std::forward<Function>(f),
			 capture = AbandonOnDrop{result}](boost::asio::yield_context yc) mutable {
				try {
					if constexpr (std::is_void_v<RetType>) {
						f(yc);
						result->SetValue();
					} else {
						result->SetValue(f(yc));
					}
				} catch (const std::exception&) {
					result->SetException(std::current_exception());
				} catch (...) {
					try {
						Log(LogCritical, "IoEngine", "Exception in coroutine!");
					} catch (...) {
					}

					// Required for proper stack unwinding when coroutines are destroyed.
					// https://github.com/boostorg/coroutine/issues/39
					throw;
				}
			}
		);

		return result;
	}

private:
	template<class Function>
	using CoroutineResultType = std::invoke_result_t<Function&, boost::asio::yield_context&>;

	/**
	 * Hands the fully wrapped coroutine body to whichever boost::asio::spawn() overload we have.
	 */
	template<class Handler, class Body>
	static void SpawnCoroutineImpl(Handler& h, Body&& wrapper)
	{
#if BOOST_VERSION >= 108700
		boost::asio::spawn(h,
			std::allocator_arg, boost::context::protected_fixedsize_stack(GetCoroutineStackSize()),
			std::forward<Body>(wrapper),
			boost::asio::detached
		);
#else // BOOST_VERSION >= 108700
		boost::asio::spawn(h, std::move(wrapper), boost::coroutines::attributes(GetCoroutineStackSize()));
#endif // BOOST_VERSION >= 108700
	}

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
	boost::asio::steady_timer m_Timer;
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
 * This class uses a boost::asio::steady_timer to emulate a timeout by scheduling a callback to be triggered
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
	using Timer = boost::asio::steady_timer;

	/**
	 * Schedules onTimeout to be triggered after timeoutFromNow on strand.
	 *
	 * @param strand The strand in which the callback will be executed.
	 *				 The caller must also run in this strand, as well as Cancel() and the destructor!
	 * @param timeoutFromNow The duration after which the timeout callback will be triggered.
	 * @param onTimeout The callback to invoke when the timeout occurs.
	 */
	template<class OnTimeout, class Rep, class Period>
	Timeout(
		boost::asio::io_context::strand& strand,
		const std::chrono::duration<Rep, Period>& timeoutFromNow,
		OnTimeout onTimeout
	)
		: m_Timer(strand.context(), std::chrono::duration_cast<Timer::duration>(timeoutFromNow)),
		  m_Cancelled(Shared<Atomic<bool>>::Make(false))
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
