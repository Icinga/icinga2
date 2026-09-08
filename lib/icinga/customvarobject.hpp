// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CUSTOMVAROBJECT_H
#define CUSTOMVAROBJECT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/customvarobject-ti.hpp"
#include "base/configobject.hpp"
#include "base/objectlock.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * An object with custom variable attribute.
 *
 * @ingroup icinga
 */
class CustomVarObject : public ObjectImpl<CustomVarObject>
{
public:
	DECLARE_OBJECT(CustomVarObject);

	void ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) final;
};

template<typename FilterMap>
int FilterArrayToInt(const Array::Ptr& typeFilters, const FilterMap& filterMap, int defaultValue)
{
	int resultTypeFilter;

	if (!typeFilters)
		return defaultValue;

	resultTypeFilter = 0;

	ObjectLock olock(typeFilters);
	for (auto& typeFilter : typeFilters) {
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

}

#endif /* CUSTOMVAROBJECT_H */
