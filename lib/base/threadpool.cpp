// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/threadpool.hpp"
#include <boost/thread/locks.hpp>

using namespace icinga;

ThreadPool::ThreadPool() : m_Pending(0)
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
		InitializePool();
	}
}

void ThreadPool::InitializePool()
{
	m_Pool = decltype(m_Pool)(new boost::asio::thread_pool(Configuration::Concurrency * 2u));
}

void ThreadPool::Stop()
{
	boost::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

	if (m_Pool) {
		m_Pool->join();
		m_Pool = nullptr;
	}
}

void ThreadPool::Restart()
{
	boost::unique_lock<decltype(m_Mutex)> lock (m_Mutex);

	if (m_Pool) {
		m_Pool->join();
	}

	InitializePool();
}
