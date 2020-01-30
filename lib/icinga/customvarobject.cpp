/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/customvarobject.hpp"
#include "icinga/customvarobject-ti.cpp"
#include "icinga/macroprocessor.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

REGISTER_TYPE(CustomVarObject);

void CustomVarObject::ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	MacroProcessor::ValidateCustomVars(this, lvalue());
}

int icinga::FilterArrayToInt(const Array::Ptr& typeFilters, const std::map<String, int>& filterMap, int defaultValue)
{
	int resultTypeFilter;

	if (!typeFilters)
		return defaultValue;

	resultTypeFilter = 0;

	ObjectLock olock(typeFilters);
	for (const Value& typeFilter : typeFilters) {
		if (typeFilter.IsNumber()) {
			resultTypeFilter = resultTypeFilter | typeFilter;
			continue;
		}

		if (!typeFilter.IsString())
			return -1;

		auto it = filterMap.find(typeFilter);

		if (it == filterMap.end())
			return -1;

		resultTypeFilter = resultTypeFilter | it->second;
	}

	return resultTypeFilter;
}
