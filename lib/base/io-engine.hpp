/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef IO_ENGINE_H
#define IO_ENGINE_H

#include "base/lazy-init.hpp"
#include "base/yield-context.hpp"
#include <atomic>
#include <exception>
#include <memory>
#include <thread>
#include <vector>
#include <stdexcept>
#include <boost/exception/all.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <spawn/spawn.hpp>

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

	/*
	 * Custom exceptions thrown in a Boost.Coroutine may cause stack corruption.
	 * Ensure that these are wrapped correctly.
	 *
	 * Inspired by https://github.com/niekbouman/commelec-api/blob/master/commelec-api/coroutine-exception.hpp
	 * Source: http://boost.2283326.n4.nabble.com/coroutine-only-std-exceptions-are-caught-from-coroutines-td4683671.html
	 */
	static inline boost::exception_ptr convertExceptionPtr(std::exception_ptr ex) {
		try {
			throw boost::enable_current_exception(ex);
		} catch (...) {
			return boost::current_exception();
		}
	}

	static inline void rethrowBoostExceptionPointer() {
		std::exception_ptr sep;
		sep = std::current_exception();
		boost::exception_ptr bep = convertExceptionPtr(sep);
		boost::rethrow_exception(bep);
	}

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

	// New Stack Allocator: Use mmap/mprotect to allocate coroutine stacks
	// Linux/Unix: 512 KB
	// Windows: 8 MB
	static boost::context::protected_fixedsize_stack MakeStackAllocator() {
#ifdef _WIN32
		return boost::context::protected_fixedsize_stack{8*1024*1024};
#else /* _WIN32 */
		return boost::context::protected_fixedsize_stack{512*1024};
#endif /* _WIN32 */
	}

	/* With dedicated strand in *Connection classes. */
	template <typename Handler, typename Function>
	static void SpawnCoroutine(Handler h, Function f) {

		spawn::spawn(std::forward<Handler>(h),
			[f](YieldContext yc) {

				try {
					f(yc);
				} catch (const boost::coroutines::detail::forced_unwind &) {
					// Required for proper stack unwinding when coroutines are destroyed.
					// https://github.com/boostorg/coroutine/issues/39
					throw;
				} catch (...) {
					// Handle uncaught exceptions outside of the coroutine.
					rethrowBoostExceptionPointer();
				}
			},
			MakeStackAllocator() // Set a pre-defined stack size.
		);
	}

	/* Without strand in the IO executor's context. */
	template <typename Function>
	static void SpawnCoroutine(boost::asio::io_context& io, Function f) {

		spawn::spawn(io,
			[f](YieldContext yc) {

				try {
					f(yc);
				} catch (const boost::coroutines::detail::forced_unwind &) {
					// Required for proper stack unwinding when coroutines are destroyed.
					// https://github.com/boostorg/coroutine/issues/39
					throw;
				} catch (...) {
					// Handle uncaught exceptions outside of the coroutine.
					rethrowBoostExceptionPointer();
				}
			},
			MakeStackAllocator() // Set a pre-defined stack size.
		);
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

}

#endif /* IO_ENGINE_H */
