/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef PERFDATAVALUE_H
#define PERFDATAVALUE_H

#include "base/i2-base.hpp"
#include "base/perfdatavalue.thpp"

namespace icinga
{

/**
 * A performance data value.
 *
 * @ingroup base
 */
class PerfdataValue final : public ObjectImpl<PerfdataValue>
{
public:
	DECLARE_OBJECT(PerfdataValue);

	PerfdataValue(void);

	PerfdataValue(String label, double value, bool counter = false, const String& unit = "",
		const Value& warn = Empty, const Value& crit = Empty,
		const Value& min = Empty, const Value& max = Empty);

	static PerfdataValue::Ptr Parse(const String& perfdata);
	String Format(void) const;

private:
	static Value ParseWarnCritMinMaxToken(const std::vector<String>& tokens,
		std::vector<String>::size_type index, const String& description);
};

}

#endif /* PERFDATA_VALUE */
