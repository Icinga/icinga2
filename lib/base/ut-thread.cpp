/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/ut-thread.hpp"

using namespace icinga;

bool UT::Thread::Resume()
{
	m_Context = m_Context.resume();

	return (bool)m_Context;
}
