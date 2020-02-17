/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/atomic.hpp"
#include "base/ut-current.hpp"
#include "base/ut-mgmt.hpp"
#include "base/ut-mutex.hpp"
#include "base/ut-thread.hpp"
#include <cstdint>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

namespace icinga
{
namespace UT
{

Aware::Mutex l_ChangeKernelspaceThreads;
Atomic<uint_fast32_t> l_KernelspaceThreads (0);
Atomic<uint_fast32_t> l_WantLessKernelspaceThreads (0);
Atomic<uint_fast64_t> l_UserspaceThreads (0);

template<bool IsMainKernelspaceThread>
void Host()
{
	l_KernelspaceThreads.fetch_add(1);

	while (l_UserspaceThreads.load()) {
		if (!IsMainKernelspaceThread) {
			auto wantLessKernelspaceThreads (l_WantLessKernelspaceThreads.exchange(0));

			if (wantLessKernelspaceThreads) {
				l_WantLessKernelspaceThreads.fetch_add(wantLessKernelspaceThreads - 1u);
				break;
			}
		}

		auto next (Queue::Default.Pop());

		if (next && next->Resume()) {
			Queue::Default.Push(std::move(next));
		}
	}

	l_KernelspaceThreads.fetch_sub(1);
}

template void Host<false>();
template void Host<true>();

void ChangeKernelspaceThreads(uint_fast32_t want)
{
	std::unique_lock<Aware::Mutex> lock (l_ChangeKernelspaceThreads);

	auto kernelspaceThreads (l_KernelspaceThreads.load());

	if (kernelspaceThreads < want) {
		for (auto diff (want - kernelspaceThreads); diff; --diff) {
			std::thread(&Host<false>).detach();
		}
	} else if (kernelspaceThreads > want) {
		l_WantLessKernelspaceThreads.fetch_add(kernelspaceThreads - want);
	} else {
		return;
	}

	while (l_KernelspaceThreads.load() != want) {
		Current::Yield_();
	}
}

Queue Queue::Default;

void Queue::Push(Thread::Ptr thread)
{
	for (;;) {
		std::unique_lock<decltype(m_Mutex)> lock (m_Mutex, std::try_to_lock);

		if (lock) {
			try {
				m_Items.emplace(std::move(thread));
			} catch (const std::bad_alloc&) {
				lock.unlock();

				if (thread->Resume()) {
					continue;
				}
			}
		} else if (thread->Resume()) {
			continue;
		}

		break;
	}
}

Thread::Ptr Queue::Pop()
{
	std::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

	if (m_Items.empty()) {
		return nullptr;
	}

	auto next (std::move(m_Items.front()));
	m_Items.pop();
	return std::move(next);
}

}
}
