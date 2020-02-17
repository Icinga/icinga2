/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/userspace-thread.hpp"

using namespace icinga;

bool UserspaceThread::Resume()
{
	m_Context = m_Context.resume();

	return (bool)m_Context;
}
