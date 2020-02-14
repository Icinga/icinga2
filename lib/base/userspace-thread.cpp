/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/atomic.hpp"
#include "base/socket.hpp"
#include "base/userspace-thread.hpp"
#include "base/ut-current.hpp"
#include "base/ut-id.hpp"
#include <boost/context/continuation.hpp>
#include <cstdint>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

#ifndef _WIN32
#	include <unistd.h>
#endif /* _WIN32 */

using namespace icinga;

UserspaceThread::Mutex UserspaceThread::m_ChangeKernelspaceThreads;
Atomic<uint_fast32_t> UserspaceThread::m_KernelspaceThreads (0);
Atomic<uint_fast32_t> UserspaceThread::m_WantLessKernelspaceThreads (0);
Atomic<uint_fast64_t> UserspaceThread::m_UserspaceThreads (0);

thread_local std::unordered_map<void*, SharedObject::Ptr> UserspaceThread::m_KernelspaceThreadLocals;

#ifndef _WIN32

decltype(fork()) UserspaceThread::Fork()
{
	auto kernelspaceThreads (m_KernelspaceThreads.load());

	ChangeKernelspaceThreads(1);

	auto pid (fork());

	ChangeKernelspaceThreads(kernelspaceThreads);

	return pid;
}

#endif /* _WIN32 */

bool UserspaceThread::Resume()
{
	m_Context = m_Context.resume();

	return (bool)m_Context;
}

void UserspaceThread::ChangeKernelspaceThreads(uint_fast32_t want)
{
	std::unique_lock<Mutex> lock (m_ChangeKernelspaceThreads);

	auto kernelspaceThreads (m_KernelspaceThreads.load());

	if (kernelspaceThreads < want) {
		for (auto diff (want - kernelspaceThreads); diff; --diff) {
			std::thread(&UserspaceThread::Host<false>).detach();
		}
	} else if (kernelspaceThreads > want) {
		m_WantLessKernelspaceThreads.fetch_add(kernelspaceThreads - want);
	} else {
		return;
	}

	while (m_KernelspaceThreads.load() != want) {
		UT::Current::Yield_();
	}
}

UserspaceThread::Queue UserspaceThread::Queue::Default;

void UserspaceThread::Queue::Push(UserspaceThread::Ptr thread)
{
	for (;;) {
		std::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

		try {
			m_Items.emplace(std::move(thread));
		} catch (const std::bad_alloc&) {
			lock.unlock();

			if (thread->Resume()) {
				continue;
			}
		}

		break;
	}
}

UserspaceThread::Ptr UserspaceThread::Queue::Pop()
{
	std::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

	if (m_Items.empty()) {
		return nullptr;
	}

	auto next (std::move(m_Items.front()));
	m_Items.pop();
	return std::move(next);
}

void UserspaceThread::Mutex::lock()
{
	while (!try_lock()) {
		UT::Current::Yield_();
	}
}

UserspaceThread::RecursiveMutex::RecursiveMutex() : m_Depth(0)
{
	m_KernelspaceOwner.store(std::thread::id());
	m_UserspaceOwner.store(UT::None);
}

void UserspaceThread::RecursiveMutex::lock()
{
	auto ust (UT::Current::GetID());

	if (ust == UT::None) {
		auto me (std::this_thread::get_id());

		if (m_KernelspaceOwner.load() == me) {
			++m_Depth;
		} else {
			m_Mutex.lock();
			m_KernelspaceOwner.store(std::move(me));
			m_Depth = 1;
		}
	} else {
		if (m_UserspaceOwner.load() == ust) {
			++m_Depth;
		} else {
			m_Mutex.lock();
			m_UserspaceOwner.store(ust);
			m_Depth = 1;
		}
	}
}

bool UserspaceThread::RecursiveMutex::try_lock()
{
	auto ust (UT::Current::GetID());

	if (ust == UT::None) {
		auto me (std::this_thread::get_id());

		if (m_KernelspaceOwner.load() == me) {
			++m_Depth;
		} else {
			if (!m_Mutex.try_lock()) {
				return false;
			}

			m_KernelspaceOwner.store(std::move(me));
			m_Depth = 1;
		}
	} else {
		if (m_UserspaceOwner.load() == ust) {
			++m_Depth;
		} else {
			if (!m_Mutex.try_lock()) {
				return false;
			}

			m_UserspaceOwner.store(ust);
			m_Depth = 1;
		}
	}

	return true;
}

void UserspaceThread::RecursiveMutex::unlock()
{
	if (!--m_Depth) {
		if (UT::Current::GetID() == UT::None) {
			m_KernelspaceOwner.store(std::thread::id());
		} else {
			m_UserspaceOwner.store(UT::None);
		}

		m_Mutex.unlock();
	}
}
