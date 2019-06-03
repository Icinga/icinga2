/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/threadpool.hpp"
#include <boost/thread/locks.hpp>

using namespace icinga;

ThreadPool::ThreadPool(size_t threads)
	: m_Threads(threads)
{
	Start();
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start()
{
	boost::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

	if (!m_Pool) {
		m_Pool = decltype(m_Pool)(new boost::asio::thread_pool(m_Threads));
	}
}

void ThreadPool::Stop()
{
	boost::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

	if (m_Pool) {
		m_Pool->join();
		m_Pool = nullptr;
	}
}
