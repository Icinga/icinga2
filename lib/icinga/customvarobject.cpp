// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/customvarobject.hpp"
#include "icinga/customvarobject-ti.cpp"
#include "icinga/macroprocessor.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

REGISTER_TYPE(CustomVarObject);

void CustomVarObject::ValidateDepthLimit(const Value& value, std::vector<String>& path)
{
	if (path.size() > VarDepthLimit) {
		BOOST_THROW_EXCEPTION(ValidationError(this, path, "Variables nested too deep."));
	}

	if (value.IsObject()) {
		Object::Ptr obj = value;

		if (auto dict = dynamic_pointer_cast<Dictionary>(obj); dict) {
			ObjectLock olock(dict);
			for (const auto &[k, v] : dict) {
				path.emplace_back(k);
				ValidateDepthLimit(v, path);
				path.pop_back();
			}
		}

		if (auto array = dynamic_pointer_cast<Array>(obj); array) {
			ObjectLock olock(array);
			std::size_t index = 0;
			for (const auto &v : array) {
				path.emplace_back(std::to_string(index++));
				ValidateDepthLimit(v, path);
				path.pop_back();
			}
		}
	}
}

void CustomVarObject::ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils&)
{
	MacroProcessor::ValidateCustomVars(this, lvalue());

	std::vector<String> path {"vars"};
	ValidateDepthLimit(lvalue(), path);
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
