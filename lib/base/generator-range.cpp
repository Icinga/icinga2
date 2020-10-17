/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/generator-range.hpp"
#include "base/objectlock.hpp"
#include "base/value.hpp"

using namespace icinga;

bool GeneratorRange::GetNext(Value& out)
{
	ObjectLock oLock (this);

	if (m_Current < m_Stop) {
		out = m_Current;
		m_Current += m_Step;
		return true;
	}

	return false;
}
