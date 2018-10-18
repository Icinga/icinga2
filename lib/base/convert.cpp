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

#include "base/convert.hpp"
#include "base/datetime.hpp"
#include <boost/lexical_cast.hpp>

using namespace icinga;

String Convert::ToString(const String& val)
{
	return val;
}

String Convert::ToString(const Value& val)
{
	return val;
}

String Convert::ToString(double val)
{
	double integral;
	double fractional = std::modf(val, &integral);

	if (fractional == 0)
		return Convert::ToString(static_cast<long long>(val));

	std::ostringstream msgbuf;
	msgbuf << std::fixed << val;
	return msgbuf.str();
}

double Convert::ToDateTimeValue(double val)
{
	return val;
}

double Convert::ToDateTimeValue(const Value& val)
{
	if (val.IsNumber())
		return val;
	else if (val.IsObjectType<DateTime>())
		return static_cast<DateTime::Ptr>(val)->GetValue();
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Not a DateTime value."));
}
