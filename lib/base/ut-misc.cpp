/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/ut-mgmt.hpp"

#ifndef _WIN32
#	include <unistd.h>
#endif /* _WIN32 */

namespace icinga
{
namespace UT
{
namespace Aware
{

#ifndef _WIN32

decltype(fork()) Fork()
{
	auto kernelspaceThreads (UT::l_KernelspaceThreads.load());

	UT::ChangeKernelspaceThreads(1);

	auto pid (fork());

	UT::ChangeKernelspaceThreads(kernelspaceThreads);

	return pid;
}

}
}
}

#endif /* _WIN32 */
