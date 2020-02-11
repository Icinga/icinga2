/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/userspace-thread.hpp"
#include <boost/context/continuation.hpp>
#include <thread>
#include <utility>

using namespace icinga;

thread_local boost::context::continuation* UserspaceThread::m_Parent = nullptr;

void UserspaceThread::Yield_()
{
	if (m_Parent != nullptr) {
		auto& parent (*m_Parent);

		m_Parent = nullptr;
		parent = parent.resume();
		m_Parent = &parent;
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
