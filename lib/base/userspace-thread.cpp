/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/userspace-thread.hpp"
#include <boost/context/continuation.hpp>

using namespace icinga;

thread_local boost::context::continuation* UserspaceThread::m_Parent = nullptr;

void UserspaceThread::Yield_()
{
	if (m_Parent != nullptr) {
		auto& parent (*m_Parent);

		m_Parent = nullptr;
		parent = parent.resume();
		m_Parent = &parent;
	}
}
