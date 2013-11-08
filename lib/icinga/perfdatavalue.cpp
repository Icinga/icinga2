#include "icinga/perfdatavalue.h"
#include "base/convert.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

PerfdataValue::PerfdataValue(double value, bool counter, const String& unit,
    const Value& warn, const Value& crit, const Value& min, const Value& max)
{
	SetValue(value);
	SetCounter(counter);
	SetUnit(unit);
	SetWarn(warn);
	SetCrit(crit);
	SetMin(min);
	SetMax(max);
}

Value PerfdataValue::Parse(const String& perfdata)
{
	size_t pos = perfdata.FindFirstNotOf("+-0123456789.e");

	double value = Convert::ToDouble(perfdata.SubStr(0, pos));

	if (pos == String::NPos)
		return value;

	std::vector<String> tokens;
	boost::algorithm::split(tokens, perfdata, boost::is_any_of(";"));

	bool counter = false;
	String unit;
	Value warn, crit, min, max;

	unit = perfdata.SubStr(pos, tokens[0].GetLength() - pos);

	boost::algorithm::to_lower(unit);

	if (unit == "us") {
		value /= 1000 * 1000;
		unit = "seconds";
	} else if (unit == "ms") {
		value /= 1000;
		unit = "seconds";
	} else if (unit == "s") {
		unit = "seconds";
	} else if (unit == "tb") {
		value *= 1024 * 1024 * 1024 * 1024;
		unit = "bytes";
	} else if (unit == "gb") {
		value *= 1024 * 1024 * 1024;
		unit = "bytes";
	} else if (unit == "mb") {
		value *= 1024 * 1024;
		unit = "bytes";
	} else if (unit == "kb") {
		value *= 1024;
		unit = "bytes";
	} else if (unit == "b") {
		unit = "bytes";
	} else if (unit == "%") {
		unit = "percent";
	} else if (unit == "c") {
		counter = true;
		unit = "";
	} else if (unit != "") {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid performance data unit: " + unit));
	}

	if (tokens.size() > 1 && tokens[1] != "U")
		warn = Convert::ToDouble(tokens[1]);

	if (tokens.size() > 2 && tokens[2] != "U")
		crit = Convert::ToDouble(tokens[2]);

	if (tokens.size() > 3 && tokens[3] != "U")
		min = Convert::ToDouble(tokens[3]);

	if (tokens.size() > 4 && tokens[4] != "U")
		max = Convert::ToDouble(tokens[4]);

	return make_shared<PerfdataValue>(value, counter, unit, warn, crit, min, max);
}

String PerfdataValue::Format(const Value& perfdata)
{
	if (perfdata.IsObjectType<PerfdataValue>()) {
		PerfdataValue::Ptr pdv = perfdata;

		String output = Convert::ToString(pdv->GetValue());

		String unit;

		if (pdv->GetCounter())
			unit = "c";
		else if (pdv->GetUnit() == "seconds")
			unit = "s";
		else if (pdv->GetUnit() == "percent")
			unit = "%";
		else if (pdv->GetUnit() == "bytes")
			unit = "B";

		output += unit;

		if (!pdv->GetWarn().IsEmpty()) {
			output += ";" + pdv->GetWarn();

			if (!pdv->GetCrit().IsEmpty()) {
				output += ";" + pdv->GetCrit();

				if (!pdv->GetMin().IsEmpty()) {
					output += ";" + pdv->GetMin();

					if (!pdv->GetMax().IsEmpty()) {
						output += ";" + pdv->GetMax();
					}
				}
			}
		}

		return output;
	} else {
		return perfdata;
	}
}
