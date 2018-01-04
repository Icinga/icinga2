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

#include "base/datetime.hpp"
#include "base/datetime.tcpp"
#include "base/utility.hpp"
#include "base/primitivetype.hpp"

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(DateTime, DateTime::GetPrototype());

DateTime::DateTime(double value)
	: m_Value(value)
{ }

DateTime::DateTime(const std::vector<Value>& args)
{
	if (args.empty())
		m_Value = Utility::GetTime();
	else if (args.size() == 3 || args.size() == 6) {
		struct tm tms;
		tms.tm_year = args[0] - 1900;
		tms.tm_mon = args[1] - 1;
		tms.tm_mday = args[2];

		if (args.size() == 6) {
			tms.tm_hour = args[3];
			tms.tm_min = args[4];
			tms.tm_sec = args[5];
		} else {
			tms.tm_hour = 0;
			tms.tm_min = 0;
			tms.tm_sec = 0;
		}

		tms.tm_isdst = -1;

		m_Value = mktime(&tms);
	} else if (args.size() == 1)
		m_Value = args[0];
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid number of arguments for the DateTime constructor."));
}

double DateTime::GetValue() const
{
	return m_Value;
}

String DateTime::Format(const String& format) const
{
	return Utility::FormatDateTime(format.CStr(), m_Value);
}

String DateTime::ToString() const
{
	return Format("%Y-%m-%d %H:%M:%S %z");
}
