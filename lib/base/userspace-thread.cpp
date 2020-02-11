/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/userspace-thread.hpp"
#include <boost/context/continuation.hpp>
#include <mutex>
#include <thread>
#include <utility>

using namespace icinga;

thread_local UserspaceThread* UserspaceThread::m_Me = nullptr;
thread_local std::unordered_map<void*, SharedObject::Ptr> UserspaceThread::m_KernelspaceThreadLocals;

void UserspaceThread::Yield_()
{
	if (m_Me != nullptr) {
		auto me (m_Me);

		m_Me = nullptr;

		{
			auto& parent (*me->m_Parent);
			parent = parent.resume();
		}

		m_Me = me;
	}
}

bool UserspaceThread::Resume()
{
	m_Context = m_Context.resume();

	return (bool)m_Context;
}

UserspaceThread::Queue UserspaceThread::Queue::Default;

void UserspaceThread::Queue::Push(UserspaceThread::Ptr thread)
{
	std::unique_lock<decltype(m_Mutex)> lock (m_Mutex);
	m_Items.emplace(std::move(thread));
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

UserspaceThread::Hoster::KernelspaceThread::KernelspaceThread()
{
	m_ShuttingDown.store(false);
	m_KSThread = std::thread([this]() { Run(); });
}

UserspaceThread::Hoster::KernelspaceThread::~KernelspaceThread()
{
	m_ShuttingDown.store(true);
	m_KSThread.join();
}

void UserspaceThread::Hoster::KernelspaceThread::Run()
{
	while (!m_ShuttingDown.load()) {
		auto next (UserspaceThread::Queue::Default.Pop());

		if (next && next->Resume()) {
			UserspaceThread::Queue::Default.Push(std::move(next));
		}
	}
}

void UserspaceThread::Mutex::lock()
{
	while (!try_lock()) {
		UserspaceThread::Yield_();
	}
}

UserspaceThread::RecursiveMutex::RecursiveMutex() : m_Depth(0)
{
	m_KernelspaceOwner.store(std::thread::id());
	m_UserspaceOwner.store(UserspaceThread::None);
}

void UserspaceThread::RecursiveMutex::lock()
{
	auto ust (UserspaceThread::GetID());

	if (ust == UserspaceThread::None) {
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
	auto ust (UserspaceThread::GetID());

	if (ust == UserspaceThread::None) {
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
		if (UserspaceThread::GetID() == UserspaceThread::None) {
			m_KernelspaceOwner.store(std::thread::id());
		} else {
			m_UserspaceOwner.store(UserspaceThread::None);
		}

		m_Mutex.unlock();
	}
}
