/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/generator-unique.hpp"
#include "base/objectlock.hpp"
#include "base/value.hpp"
#include <utility>

using namespace icinga;

bool GeneratorUnique::GetNext(Value& out)
{
	ObjectLock oLock (this);

	Value buf;

	while (m_Source->GetNext(buf)) {
		auto res (m_Unique.emplace(std::move(buf)));

		if (res.second) {
			out = *res.first;
			return true;
		}
	}

	return false;
}
