/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-icinga.h"

using namespace icinga;

static AttributeDescription timePeriodAttributes[] = {
	{ "alias", Attribute_Config },
	{ "cached_state", Attribute_Transient },
	{ "cached_next_change", Attribute_Transient }
};

REGISTER_TYPE(TimePeriod, timePeriodAttributes);

String TimePeriod::GetAlias(void) const
{
	String value = Get("alias");

	if (!value.IsEmpty())
		return value;
	else
		return GetName();
}

bool TimePeriod::Exists(const String& name)
{
	return (DynamicObject::GetObject("TimePeriod", name));
}

TimePeriod::Ptr TimePeriod::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("TimePeriod", name);

	if (!configObject)
		throw_exception(invalid_argument("TimePeriod '" + name + "' does not exist."));

	return dynamic_pointer_cast<TimePeriod>(configObject);
}

bool TimePeriod::IsActive(void) {
	if (GetNextChange() > Utility::GetTime()) {
		vector<Value> args;
		args.push_back(static_cast<TimePeriod::Ptr>(GetSelf()));
		return InvokeMethodSync<bool>("is_active", args);
	} else {
		return Get("cached_state");
	}
}

double TimePeriod::GetNextChange(void) {
	double next_change = Get("cached_next_change");

	if (next_change < Utility::GetTime()) {
		vector<Value> args;
		args.push_back(static_cast<TimePeriod::Ptr>(GetSelf()));
		next_change = InvokeMethodSync<bool>("is_active", args);
		// TODO: figure out next_change by calling method

		Set("cached_next_change", next_change);
	}

	return next_change;
}
