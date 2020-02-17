/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/ut-thread.hpp"

namespace icinga
{
namespace UT
{

bool Thread::Resume()
{
	m_Context = m_Context.resume();

	return (bool)m_Context;
}

}
}
