/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/datetime.hpp"
#include "base/datetime-ti.cpp"
#include "base/locale.hpp"
#include "base/utility.hpp"
#include "base/primitivetype.hpp"
#include <boost/locale.hpp>

using namespace icinga;

namespace lc = boost::locale;

REGISTER_TYPE_WITH_PROTOTYPE(DateTime, DateTime::GetPrototype());

DateTime::DateTime(double value)
	: m_Value(value)
{ }

DateTime::DateTime(const std::vector<Value>& args)
{
	if (args.empty())
		m_Value = Utility::GetTime();
	else if (args.size() == 3 || args.size() == 6) {
		LocaleDateTime dt;

		dt.set(lc::period::year(), args[0]);
		dt.set(lc::period::month(), args[1] - 1);
		dt.set(lc::period::day(), args[2]);

		if (args.size() == 6) {
			dt.set(lc::period::hour(), args[3]);
			dt.set(lc::period::minute(), args[4]);
			dt.set(lc::period::second(), args[5]);
		} else {
			dt.set(lc::period::hour(), 0);
			dt.set(lc::period::minute(), 0);
			dt.set(lc::period::second(), 0);
		}

		m_Value = dt.time();
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
