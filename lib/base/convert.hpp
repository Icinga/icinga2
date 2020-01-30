/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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

	static long ToLong(double val)
	{
		return static_cast<long>(val);
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
