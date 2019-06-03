/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/perfdatavalue.hpp"
#include "base/perfdatavalue-ti.cpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"

using namespace icinga;

REGISTER_TYPE(PerfdataValue);
REGISTER_FUNCTION(System, parse_performance_data, PerfdataValue::Parse, "perfdata");

PerfdataValue::PerfdataValue(const String& label, double value, bool counter,
	const String& unit, const Value& warn, const Value& crit, const Value& min,
	const Value& max)
{
	SetLabel(label, true);
	SetValue(value, true);
	SetCounter(counter, true);
	SetUnit(unit, true);
	SetWarn(warn, true);
	SetCrit(crit, true);
	SetMin(min, true);
	SetMax(max, true);
}

PerfdataValue::Ptr PerfdataValue::Parse(const String& perfdata)
{
	size_t eqp = perfdata.FindLastOf('=');

	if (eqp == String::NPos)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid performance data value: " + perfdata));

	String label = perfdata.SubStr(0, eqp);

	if (label.GetLength() > 2 && label[0] == '\'' && label[label.GetLength() - 1] == '\'')
		label = label.SubStr(1, label.GetLength() - 2);

	size_t spq = perfdata.FindFirstOf(' ', eqp);

	if (spq == String::NPos)
		spq = perfdata.GetLength();

	String valueStr = perfdata.SubStr(eqp + 1, spq - eqp - 1);

	size_t pos = valueStr.FindFirstNotOf("+-0123456789.e");

	double value = Convert::ToDouble(valueStr.SubStr(0, pos));

	std::vector<String> tokens = valueStr.Split(";");

	bool counter = false;
	String unit;
	Value warn, crit, min, max;

	if (pos != String::NPos)
		unit = valueStr.SubStr(pos, tokens[0].GetLength() - pos);

	unit = unit.ToLower();

	double base = 1.0;

	if (unit == "us") {
		base /= 1000.0 * 1000.0;
		unit = "seconds";
	} else if (unit == "ms") {
		base /= 1000.0;
		unit = "seconds";
	} else if (unit == "s") {
		unit = "seconds";
	} else if (unit == "tb") {
		base *= 1024.0 * 1024.0 * 1024.0 * 1024.0;
		unit = "bytes";
	} else if (unit == "gb") {
		base *= 1024.0 * 1024.0 * 1024.0;
		unit = "bytes";
	} else if (unit == "mb") {
		base *= 1024.0 * 1024.0;
		unit = "bytes";
	} else if (unit == "kb") {
		base *= 1024.0;
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

	warn = ParseWarnCritMinMaxToken(tokens, 1, "warning");
	crit = ParseWarnCritMinMaxToken(tokens, 2, "critical");
	min = ParseWarnCritMinMaxToken(tokens, 3, "minimum");
	max = ParseWarnCritMinMaxToken(tokens, 4, "maximum");

	value = value * base;

	if (!warn.IsEmpty())
		warn = warn * base;

	if (!crit.IsEmpty())
		crit = crit * base;

	if (!min.IsEmpty())
		min = min * base;

	if (!max.IsEmpty())
		max = max * base;

	return new PerfdataValue(label, value, counter, unit, warn, crit, min, max);
}

String PerfdataValue::Format() const
{
	std::ostringstream result;

	if (GetLabel().FindFirstOf(" ") != String::NPos)
		result << "'" << GetLabel() << "'";
	else
		result << GetLabel();

	result << "=" << Convert::ToString(GetValue());

	String unit;

	if (GetCounter())
		unit = "c";
	else if (GetUnit() == "seconds")
		unit = "s";
	else if (GetUnit() == "percent")
		unit = "%";
	else if (GetUnit() == "bytes")
		unit = "B";

	result << unit;

	if (!GetWarn().IsEmpty()) {
		result << ";" << Convert::ToString(GetWarn());

		if (!GetCrit().IsEmpty()) {
			result << ";" << Convert::ToString(GetCrit());

			if (!GetMin().IsEmpty()) {
				result << ";" << Convert::ToString(GetMin());

				if (!GetMax().IsEmpty()) {
					result << ";" << Convert::ToString(GetMax());
				}
			}
		}
	}

	return result.str();
}

Value PerfdataValue::ParseWarnCritMinMaxToken(const std::vector<String>& tokens, std::vector<String>::size_type index, const String& description)
{
	if (tokens.size() > index && tokens[index] != "U" && tokens[index] != "" && tokens[index].FindFirstNotOf("+-0123456789.e") == String::NPos)
		return Convert::ToDouble(tokens[index]);
	else {
		if (tokens.size() > index && tokens[index] != "")
			Log(LogDebug, "PerfdataValue")
				<< "Ignoring unsupported perfdata " << description << " range, value: '" << tokens[index] << "'.";
		return Empty;
	}
}
