/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/generator-map.hpp"
#include "base/value.hpp"
#include <utility>

using namespace icinga;

bool GeneratorMap::GetNext(Value& out)
{
	Value buf;

	if (m_Source->GetNext(buf)) {
		out = m_Function->Invoke({ std::move(buf) });
		return true;
	}

	return false;
}
