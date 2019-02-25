/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
