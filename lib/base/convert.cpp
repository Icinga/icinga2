// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/convert.hpp"
#include "base/datetime.hpp"
#include <boost/lexical_cast.hpp>
#include <iomanip>

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

	std::ostringstream msgbuf;
	if (fractional == 0) {
		msgbuf << std::setprecision(0);
	}
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
