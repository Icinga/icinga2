/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/datetime.hpp"
#include "base/datetime-ti.cpp"
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

		m_Value = Utility::TmToTimestamp(&tms);
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
