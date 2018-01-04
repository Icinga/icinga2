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

#ifndef CONVERT_H
#define CONVERT_H

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include <boost/lexical_cast.hpp>

namespace icinga
{

/**
 * Utility class for converting types.
 *
 * @ingroup base
 */
class Convert
{
public:
	template<typename T>
	static long ToLong(const T& val)
	{
		try {
			return boost::lexical_cast<long>(val);
		} catch (const std::exception&) {
			std::ostringstream msgbuf;
			msgbuf << "Can't convert '" << val << "' to an integer.";
			BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
		}
	}

	template<typename T>
	static double ToDouble(const T& val)
	{
		try {
			return boost::lexical_cast<double>(val);
		} catch (const std::exception&) {
			std::ostringstream msgbuf;
			msgbuf << "Can't convert '" << val << "' to a floating point number.";
			BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
		}
	}

	static long ToLong(const Value& val)
	{
		return val;
	}

	static double ToDouble(const Value& val)
	{
		return val;
	}

	static bool ToBool(const Value& val)
	{
		return val.ToBool();
	}

	template<typename T>
	static String ToString(const T& val)
	{
		return boost::lexical_cast<std::string>(val);
	}

	static String ToString(const String& val);
	static String ToString(const Value& val);
	static String ToString(double val);

	static double ToDateTimeValue(double val);
	static double ToDateTimeValue(const Value& val);

private:
	Convert();
};

}

#endif /* CONVERT_H */
