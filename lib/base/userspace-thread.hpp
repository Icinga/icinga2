/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef USERSPACE_THREAD_H
#define USERSPACE_THREAD_H

#include "base/atomic.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/object.hpp"
#include "base/shared-object.hpp"
#include "base/ut-current.hpp"
#include "base/ut-mutex.hpp"
#include <atomic>
#include <boost/context/continuation.hpp>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <utility>

#ifndef _WIN32
#	include <unistd.h>
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
	class Queue;

	template<class T>
	class Local;

	template<class T>
	friend class Local;

	DECLARE_PTR_TYPEDEFS(UserspaceThread);

	static inline
	void Main()
	{
		Host<true>();
	}

#ifndef _WIN32
	static decltype(fork()) Fork();
#endif /* _WIN32 */

	template<class F>
	UserspaceThread(F&& f);

	UserspaceThread(const UserspaceThread&) = delete;
	UserspaceThread(UserspaceThread&&) = delete;
	UserspaceThread& operator=(const UserspaceThread&) = delete;
	UserspaceThread& operator=(UserspaceThread&&) = delete;

	bool Resume();

	boost::context::continuation* m_Parent;

private:
	template<bool IsMainKernelspaceThread>
	static void Host();

	static void ChangeKernelspaceThreads(uint_fast32_t want);

	static UT::Aware::Mutex m_ChangeKernelspaceThreads;
	static Atomic<uint_fast32_t> m_KernelspaceThreads;
	static Atomic<uint_fast32_t> m_WantLessKernelspaceThreads;
	static Atomic<uint_fast64_t> m_UserspaceThreads;

	static thread_local std::unordered_map<void*, SharedObject::Ptr> m_KernelspaceThreadLocals;

	boost::context::continuation m_Context;
	std::unordered_map<void*, SharedObject::Ptr> m_Locals;

	template<class F>
	inline boost::context::continuation FunctionToContext(F&& f)
	{
		Ptr keepAlive (this);

		return boost::context::callcc([this, keepAlive, f](boost::context::continuation&& parent) {
			m_UserspaceThreads.fetch_add(1);
			m_Parent = &parent;
			UT::Current::m_Thread = this;
			UT::Current::Yield_();

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

			UT::Current::m_Thread = nullptr;
			m_UserspaceThreads.fetch_sub(1);

			return std::move(parent);
		});
	}
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
	UT::Aware::Mutex m_Mutex;
};

template<class F>
UserspaceThread::UserspaceThread(F&& f)
	: m_Parent(nullptr), m_Context(FunctionToContext(std::move(f)))
{
	UserspaceThread::Queue::Default.Push(this);
}

template<bool IsMainKernelspaceThread>
void UserspaceThread::Host()
{
	m_KernelspaceThreads.fetch_add(1);

	while (m_UserspaceThreads.load()) {
		if (!IsMainKernelspaceThread) {
			auto wantLessKernelspaceThreads (m_WantLessKernelspaceThreads.exchange(0));

			if (wantLessKernelspaceThreads) {
				m_WantLessKernelspaceThreads.fetch_add(wantLessKernelspaceThreads - 1u);
				break;
			}
		}

		auto next (UserspaceThread::Queue::Default.Pop());

		if (next && next->Resume()) {
			UserspaceThread::Queue::Default.Push(std::move(next));
		}
	}

	m_KernelspaceThreads.fetch_sub(1);
}

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
		auto locals (UT::Current::m_Thread == nullptr ? &UserspaceThread::m_KernelspaceThreadLocals : &UT::Current::m_Thread->m_Locals);
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

}

#endif /* USERSPACE_THREAD_H */
