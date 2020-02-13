/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef USERSPACE_THREAD_H
#define USERSPACE_THREAD_H

#include "base/configuration.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/object.hpp"
#include "base/shared-object.hpp"
#include <atomic>
#include <boost/context/continuation.hpp>
#include <cstdint>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>
#include <utility>

#ifdef _WIN32

#include <winsock2.h>

#else /* _WIN32 */

#include <sys/socket.h>

#endif /* _WIN32 */

namespace icinga
{

/**
 * A lightweight thread.
 *
 * @ingroup base
 */
class UserspaceThread : public SharedObject
{
public:
	typedef void* ID;

	static constexpr ID None = nullptr;

	class Queue;
	class Hoster;
	class Mutex;
	class RecursiveMutex;

	template<class T>
	class Local;

	template<class T>
	friend class Local;

	template<class SRS>
	class SyncReadStream;

	template<class SRS>
	friend class SyncReadStream;

	template<class SWS>
	class SyncWriteStream;

	template<class SWS>
	friend class SyncWriteStream;

	DECLARE_PTR_TYPEDEFS(UserspaceThread);

	static inline
	ID GetID()
	{
		return m_Me;
	}

	static void Yield_();
	static void Main();

	template<class F>
	UserspaceThread(F&& f);

	UserspaceThread(const UserspaceThread&) = delete;
	UserspaceThread(UserspaceThread&&) = delete;
	UserspaceThread& operator=(const UserspaceThread&) = delete;
	UserspaceThread& operator=(UserspaceThread&&) = delete;

	bool Resume();

private:
	typedef decltype(socket(0, 0, 0)) NativeSocket;

	enum class SocketOp : uint_fast8_t
	{
		Read, Write
	};

	static void Host();
	static void WaitForSocket(NativeSocket sock, SocketOp op);

	static thread_local UserspaceThread* m_Me;
	static thread_local std::unordered_map<void*, SharedObject::Ptr> m_KernelspaceThreadLocals;

	boost::context::continuation* m_Parent;
	boost::context::continuation m_Context;
	std::unordered_map<void*, SharedObject::Ptr> m_Locals;

	template<class F>
	inline boost::context::continuation FunctionToContext(F&& f)
	{
		Ptr keepAlive (this);

		return boost::context::callcc([this, keepAlive, f](boost::context::continuation&& parent) {
			m_Parent = &parent;
			m_Me = this;
			Yield_();

			try {
				f();
			} catch (const std::exception& ex) {
				try {
					Log(LogCritical, "UserspaceThread")
						<< "Exception thrown in UserspaceThread:\n"
						<< DiagnosticInformation(ex);
				} catch (...) {
				}
			} catch (...) {
				try {
					Log(LogCritical, "UserspaceThread", "Exception of unknown type thrown in UserspaceThread.");
				} catch (...) {
				}
			}

			m_Me = nullptr;

			return std::move(parent);
		});
	}
};

/**
 * Collection of kernelspace-threads hosting UserspaceThreads.
 *
 * @ingroup base
 */
class UserspaceThread::Hoster : public SharedObject
{
public:
	inline Hoster(int ksThreads = Configuration::Concurrency);

	Hoster(const Hoster&) = delete;
	Hoster& operator=(const Hoster&) = delete;

private:
	class KernelspaceThread;

	std::unique_ptr<KernelspaceThread[]> m_KSThreads;
};

/**
 * A kernelspace-thread hosting UserspaceThreads.
 *
 * @ingroup base
 */
class UserspaceThread::Hoster::KernelspaceThread
{
public:
	KernelspaceThread();

	KernelspaceThread(const KernelspaceThread&) = delete;
	KernelspaceThread(KernelspaceThread&&) = delete;
	KernelspaceThread& operator=(const KernelspaceThread&) = delete;
	KernelspaceThread& operator=(KernelspaceThread&&) = delete;

	~KernelspaceThread();

private:
	void Run();

	std::thread m_KSThread;
	std::atomic<bool> m_ShuttingDown;
};

inline UserspaceThread::Hoster::Hoster(int ksThreads)
{
	m_KSThreads.reset(new KernelspaceThread[ksThreads]);
}

/**
 * Like std::mutex, but UserspaceThread-aware.
 *
 * @ingroup base
 */
class UserspaceThread::Mutex
{
public:
	inline Mutex() = default;

	Mutex(const Mutex&) = delete;
	Mutex(Mutex&&) = delete;
	Mutex& operator=(const Mutex&) = delete;
	Mutex& operator=(Mutex&&) = delete;

	void lock();

	inline bool try_lock()
	{
		return !m_Locked.test_and_set(std::memory_order_acquire);
	}

	inline void unlock()
	{
		m_Locked.clear(std::memory_order_release);
	}

private:
	std::atomic_flag m_Locked = ATOMIC_FLAG_INIT;
};

/**
 * Collection of UserspaceThreads calling UserspaceThread::Yield_().
 *
 * @ingroup base
 */
class UserspaceThread::Queue
{
public:
	static Queue Default;

	inline Queue() = default;

	Queue(const Queue&) = delete;
	Queue(Queue&&) = delete;
	Queue& operator=(const Queue&) = delete;
	Queue& operator=(Queue&&) = delete;

	void Push(UserspaceThread::Ptr thread);
	UserspaceThread::Ptr Pop();

private:
	std::queue<UserspaceThread::Ptr> m_Items;
	UserspaceThread::Mutex m_Mutex;
};

template<class F>
UserspaceThread::UserspaceThread(F&& f)
	: m_Parent(nullptr), m_Context(FunctionToContext(std::move(f)))
{
	UserspaceThread::Queue::Default.Push(this);
}

/**
 * Like std::recursive_mutex, but UserspaceThread-aware.
 *
 * @ingroup base
 */
class UserspaceThread::RecursiveMutex
{
public:
	RecursiveMutex();

	RecursiveMutex(const RecursiveMutex&) = delete;
	RecursiveMutex(RecursiveMutex&&) = delete;
	RecursiveMutex& operator=(const RecursiveMutex&) = delete;
	RecursiveMutex& operator=(RecursiveMutex&&) = delete;

	void lock();
	bool try_lock();
	void unlock();

private:
	std::atomic<std::thread::id> m_KernelspaceOwner;
	std::atomic<UserspaceThread::ID> m_UserspaceOwner;
	uint_fast32_t m_Depth;
	UserspaceThread::Mutex m_Mutex;
};

/**
 * A UserspaceThread-local variable.
 *
 * @ingroup base
 */
template<class T>
class UserspaceThread::Local
{
public:
	inline Local() = default;

	Local(const Local&) = delete;
	Local(Local&&) = delete;
	Local& operator=(const Local&) = delete;
	Local& operator=(Local&&) = delete;

	inline T& operator*()
	{
		return Get();
	}

	inline T* operator->()
	{
		return &Get();
	}

private:
	class Storage;

	T& Get()
	{
		auto locals (UserspaceThread::m_Me == nullptr ? &UserspaceThread::m_KernelspaceThreadLocals : &UserspaceThread::m_Me->m_Locals);
		auto& storage ((*locals)[this]);

		if (!storage) {
			storage = new Storage();
		}

		return static_cast<Storage*>(storage.get())->Var;
	}
};

/**
 * Storage for a UserspaceThread-local variable.
 *
 * @ingroup base
 */
template<class T>
class UserspaceThread::Local<T>::Storage : public SharedObject
{
public:
	T Var;
};

/**
 * Like SRS, but UserspaceThread-aware.
 *
 * @ingroup base
 */
template<class SRS>
class UserspaceThread::SyncReadStream : public SRS
{
public:
	using SRS::SRS;

	template<class... Args>
	auto read_some(Args&&... args) -> decltype(((SRS*)nullptr)->read_some(std::forward<Args>(args)...))
	{
		UserspaceThread::WaitForSocket(this->lowest_layer().native_handle(), UserspaceThread::SocketOp::Read);

		return ((SRS*)this)->read_some(std::forward<Args>(args)...);
	}
};

/**
 * Like SWS, but UserspaceThread-aware.
 *
 * @ingroup base
 */
template<class SWS>
class UserspaceThread::SyncWriteStream : public SWS
{
public:
	using SWS::SWS;

	template<class... Args>
	auto write_some(Args&&... args) -> decltype(((SWS*)nullptr)->write_some(std::forward<Args>(args)...))
	{
		UserspaceThread::WaitForSocket(this->lowest_layer().native_handle(), UserspaceThread::SocketOp::Write);

		return ((SWS*)this)->write_some(std::forward<Args>(args)...);
	}
};

}

#endif /* USERSPACE_THREAD_H */
