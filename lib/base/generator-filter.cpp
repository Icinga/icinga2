/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/generator-filter.hpp"
#include "base/value.hpp"
#include <utility>

using namespace icinga;

bool GeneratorFilter::GetNext(Value& out)
{
	Value buf;

	while (m_Source->GetNext(buf)) {
		if (m_Function->Invoke({ buf })) {
			out = std::move(buf);
			return true;
		}
	}

	return false;
}
