/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/ut-current.hpp"
#include "base/ut-id.hpp"
#include "base/ut-mutex.hpp"
#include <thread>
#include <utility>

using namespace icinga;

void UT::Aware::Mutex::lock()
{
	while (!try_lock()) {
		Current::Yield_();
	}
}

UT::Aware::RecursiveMutex::RecursiveMutex() : m_Depth(0)
{
	m_KernelspaceOwner.store(std::thread::id());
	m_UserspaceOwner.store(None);
}

void UT::Aware::RecursiveMutex::lock()
{
	auto ust (Current::GetID());

	if (ust == None) {
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

bool UT::Aware::RecursiveMutex::try_lock()
{
	auto ust (Current::GetID());

	if (ust == None) {
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

void UT::Aware::RecursiveMutex::unlock()
{
	if (!--m_Depth) {
		if (Current::GetID() == None) {
			m_KernelspaceOwner.store(std::thread::id());
		} else {
			m_UserspaceOwner.store(None);
		}

		m_Mutex.unlock();
	}
}
