/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/generator-array.hpp"
#include "base/objectlock.hpp"
#include "base/value.hpp"

using namespace icinga;

bool GeneratorArray::GetNext(Value& out)
{
	ObjectLock oLock (m_Source);

	if (m_Next < m_Source->GetLength()) {
		out = m_Source->Get(m_Next++);
		return true;
	}

	return false;
}
