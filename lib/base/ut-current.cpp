/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/ut-current.hpp"
#include "base/ut-thread.hpp"

using namespace icinga;

thread_local UT::Thread* UT::Current::m_Thread = nullptr;

void UT::Current::Yield_()
{
	if (m_Thread != nullptr) {
		auto me (m_Thread);

		m_Thread = nullptr;

		{
			auto& parent (*me->m_Parent);
			parent = parent.resume();
		}

		m_Thread = me;
	}
}
