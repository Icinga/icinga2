/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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

