/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_MGMT_H
#define UT_MGMT_H

#include "base/atomic.hpp"
#include "base/ut-mutex.hpp"
#include "base/ut-thread.hpp"
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <cstdint>
#include <queue>

namespace icinga
{
namespace UT
{

extern Aware::Mutex l_ChangeKernelspaceThreads;
extern Atomic<uint_fast32_t> l_KernelspaceThreads;
extern Atomic<uint_fast32_t> l_WantLessKernelspaceThreads;
extern Atomic<uint_fast64_t> l_UserspaceThreads;

/**
 * Collection of UserspaceThreads calling UT::Current::Yield_().
 *
 * @ingroup base
 */
class Queue
{
public:
	static Queue Default;

	inline Queue() = default;

	Queue(const Queue&) = delete;
	Queue(Queue&&) = delete;
	Queue& operator=(const Queue&) = delete;
	Queue& operator=(Queue&&) = delete;

	void Push(boost::intrusive_ptr<Thread> thread);
	boost::intrusive_ptr<Thread> Pop();

private:
	std::queue<boost::intrusive_ptr<Thread>> m_Items;
	UT::Aware::Mutex m_Mutex;
};

void ChangeKernelspaceThreads(uint_fast32_t want);

template<bool IsMainKernelspaceThread>
void Host();

inline void Main()
{
	Host<true>();
}

}
}

#endif /* UT_MGMT_H */
