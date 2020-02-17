/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/userspace-thread.hpp"
#include "base/ut-mgmt.hpp"

#ifndef _WIN32
#	include <unistd.h>
#endif /* _WIN32 */

using namespace icinga;

#ifndef _WIN32

decltype(fork()) UserspaceThread::Fork()
{
	auto kernelspaceThreads (UT::l_KernelspaceThreads.load());

	UT::ChangeKernelspaceThreads(1);

	auto pid (fork());

	UT::ChangeKernelspaceThreads(kernelspaceThreads);

	return pid;
}

#endif /* _WIN32 */

bool UserspaceThread::Resume()
{
	m_Context = m_Context.resume();

	return (bool)m_Context;
}
