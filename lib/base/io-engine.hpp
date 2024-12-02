/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef IO_ENGINE_H
#define IO_ENGINE_H

#include "base/defer.hpp"
#include "base/exception.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include "base/shared.hpp"
#include "base/shared-object.hpp"
#include <atomic>
#include <exception>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <stdexcept>
#include <boost/exception/all.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

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
	~CpuBoundWork();

	void Done();

private:
	bool m_Done;
};

/**
 * Scope break for CPU-bound work done in an I/O thread
 *
 * @ingroup base
 */
class IoBoundWorkSlot
{
public:
	IoBoundWorkSlot(boost::asio::yield_context yc);
	IoBoundWorkSlot(const IoBoundWorkSlot&) = delete;
	IoBoundWorkSlot(IoBoundWorkSlot&&) = delete;
	IoBoundWorkSlot& operator=(const IoBoundWorkSlot&) = delete;
	IoBoundWorkSlot& operator=(IoBoundWorkSlot&&) = delete;
	~IoBoundWorkSlot();

private:
	boost::asio::yield_context yc;
};

/**
 * Async I/O engine
 *
 * @ingroup base
 */
class IoEngine
{
	friend CpuBoundWork;
	friend IoBoundWorkSlot;

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

		boost::asio::spawn(h,
			[f](boost::asio::yield_context yc) {

				try {
					f(yc);
				} catch (const boost::coroutines::detail::forced_unwind &) {
					// Required for proper stack unwinding when coroutines are destroyed.
					// https://github.com/boostorg/coroutine/issues/39
					throw;
				} catch (const std::exception& ex) {
					Log(LogCritical, "IoEngine") << "Exception in coroutine: " << DiagnosticInformation(ex);
				} catch (...) {
					Log(LogCritical, "IoEngine", "Exception in coroutine!");
				}
			},
			boost::coroutines::attributes(GetCoroutineStackSize()) // Set a pre-defined stack size.
		);
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
 * Condition variable which doesn't block I/O threads
 *
 * @ingroup base
 */
class AsioConditionVariable
{
public:
	AsioConditionVariable(boost::asio::io_context& io, bool init = false);

	void Set();
	void Clear();
	void Wait(boost::asio::yield_context yc);

private:
	boost::asio::deadline_timer m_Timer;
};

/**
 * I/O timeout emulator
 *
 * @ingroup base
 */
class Timeout : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(Timeout);

	template<class Executor, class TimeoutFromNow, class OnTimeout>
	Timeout(boost::asio::io_context& io, Executor& executor, TimeoutFromNow timeoutFromNow, OnTimeout onTimeout)
		: m_Timer(io), m_Done(io)
	{
		Ptr keepAlive (this);

		m_Cancelled.store(false);
		m_Timer.expires_from_now(std::move(timeoutFromNow));

		auto setDone (Shared<Defer>::Make([this, keepAlive] { m_Done.Set(); }));

		IoEngine::SpawnCoroutine(executor, [this, keepAlive, setDone, onTimeout](boost::asio::yield_context yc) {
			if (m_Cancelled.load()) {
				return;
			}

			{
				boost::system::error_code ec;

				m_Timer.async_wait(yc[ec]);

				if (ec) {
					return;
				}
			}

			if (m_Cancelled.load()) {
				return;
			}

			auto f (onTimeout);
			f(std::move(yc));
		});
	}

	void Cancel();
	void Cancel(boost::asio::yield_context yc);

private:
	boost::asio::deadline_timer m_Timer;
	std::atomic<bool> m_Cancelled;
	AsioConditionVariable m_Done;
};

}

#endif /* IO_ENGINE_H */
