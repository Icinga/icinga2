/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/atomic.hpp"
#include "base/exception.hpp"
#include "base/socket.hpp"
#include "base/userspace-thread.hpp"
#include <boost/context/continuation.hpp>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>

#ifdef _WIN32

#include <winsock.h>
#include <winsock2.h>

#else /* _WIN32 */

#include <errno.h>
#include <sys/select.h>

#endif /* _WIN32 */

using namespace icinga;

UserspaceThread::Mutex UserspaceThread::m_ChangeKernelspaceThreads;
Atomic<uint_fast32_t> UserspaceThread::m_KernelspaceThreads (0);
Atomic<uint_fast32_t> UserspaceThread::m_WantLessKernelspaceThreads (0);

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
		Yield_();
	}
}

void UserspaceThread::WaitForSocket(UserspaceThread::NativeSocket sock, UserspaceThread::SocketOp op)
{
	fd_set fds;
	fd_set* reads = nullptr;
	fd_set* writes = nullptr;
	struct timeval timeout = { 0, 0 };

	switch (op) {
		case SocketOp::Read:
			reads = &fds;
			break;
		case SocketOp::Write:
			writes = &fds;
	}

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		auto readyFds (select(1, reads, writes, nullptr, &timeout));

		if (readyFds < 0u) {
#ifdef _WIN32
			BOOST_THROW_EXCEPTION(socket_error()
				<< boost::errinfo_api_function("select")
				<< errinfo_win32_error(WSAGetLastError()));
#else /* _WIN32 */
			BOOST_THROW_EXCEPTION(socket_error()
				<< boost::errinfo_api_function("select")
				<< boost::errinfo_errno(errno));
#endif /* _WIN32 */
		}

		if (readyFds) {
			break;
		}

		Yield_();
	}
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
