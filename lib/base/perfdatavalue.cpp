// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/perfdatavalue.hpp"
#include "base/perfdatavalue-ti.cpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

using namespace icinga;

REGISTER_TYPE(PerfdataValue);
REGISTER_FUNCTION(System, parse_performance_data, PerfdataValue::Parse, "perfdata");

struct UoM
{
	double Factor;
	const char* Out;
};

typedef std::unordered_map<std::string /* in */, UoM> UoMs;
typedef std::unordered_multimap<std::string /* in */, UoM> DupUoMs;

static const UoMs l_CsUoMs (([]() -> UoMs {
	DupUoMs uoms ({
		// Misc:
		{ "", { 1, "" } },
		{ "%", { 1, "percent" } },
		{ "c", { 1, "" } },
		{ "C", { 1, "degrees-celsius" } }
	});

	{
		// Data (rate):

		struct { const char* Char; int Power; } prefixes[] = {
			{ "k", 1 }, { "K", 1 },
			{ "m", 2 }, { "M", 2 },
			{ "g", 3 }, { "G", 3 },
			{ "t", 4 }, { "T", 4 },
			{ "p", 5 }, { "P", 5 },
			{ "e", 6 }, { "E", 6 },
			{ "z", 7 }, { "Z", 7 },
			{ "y", 8 }, { "Y", 8 }
		};

		struct { const char* Char; double Factor; } siIecs[] = {
			{ "", 1000 },
			{ "i", 1024 }, { "I", 1024 }
		};

		struct { const char *In, *Out; } bases[] = {
			{ "b", "bits" },
			{ "B", "bytes" }
		};

		for (auto base : bases) {
			uoms.emplace(base.In, UoM{1, base.Out});
		}

		for (auto prefix : prefixes) {
			for (auto siIec : siIecs) {
				auto factor (pow(siIec.Factor, prefix.Power));

				for (auto base : bases) {
					uoms.emplace(
						std::string(prefix.Char) + siIec.Char + base.In,
						UoM{factor, base.Out}
					);
				}
			}
		}
	}

	{
		// Energy:

		struct { const char* Char; int Power; } prefixes[] = {
			{ "n", -3 }, { "N", -3 },
			{ "u", -2 }, { "U", -2 },
			{ "m", -1 },
			{ "", 0 },
			{ "k", 1 }, { "K", 1 },
			{ "M", 2 },
			{ "g", 3 }, { "G", 3 },
			{ "t", 4 }, { "T", 4 },
			{ "p", 5 }, { "P", 5 },
			{ "e", 6 }, { "E", 6 },
			{ "z", 7 }, { "Z", 7 },
			{ "y", 8 }, { "Y", 8 }
		};

		{
			struct { const char* Ins[2]; const char* Out; } bases[] = {
				{ { "a", "A" }, "amperes" },
				{ { "o", "O" }, "ohms" },
				{ { "v", "V" }, "volts" },
				{ { "w", "W" }, "watts" }
			};

			for (auto prefix : prefixes) {
				auto factor (pow(1000.0, prefix.Power));

				for (auto base : bases) {
					for (auto b : base.Ins) {
						uoms.emplace(std::string(prefix.Char) + b, UoM{factor, base.Out});
					}
				}
			}
		}

		struct { const char* Char; double Factor; } suffixes[] = {
			{ "s", 1 }, { "S", 1 },
			{ "m", 60 }, { "M", 60 },
			{ "h", 60 * 60 }, { "H", 60 * 60 }
		};

		struct { const char* Ins[2]; double Factor; const char* Out; } bases[] = {
			{ { "a", "A" }, 1, "ampere-seconds" },
			{ { "w", "W" }, 60 * 60, "watt-hours" }
		};

		for (auto prefix : prefixes) {
			auto factor (pow(1000.0, prefix.Power));

			for (auto suffix : suffixes) {
				auto timeFactor (factor * suffix.Factor);

				for (auto& base : bases) {
					auto baseFactor (timeFactor / base.Factor);

					for (auto b : base.Ins) {
						uoms.emplace(
							std::string(prefix.Char) + b + suffix.Char,
							UoM{baseFactor, base.Out}
						);
					}
				}
			}
		}
	}

	UoMs uniqUoms;

	for (auto& uom : uoms) {
		if (!uniqUoms.emplace(uom).second) {
			throw std::logic_error("Duplicate case-sensitive UoM detected: " + uom.first);
		}
	}

	return uniqUoms;
})());

static const UoMs l_CiUoMs (([]() -> UoMs {
	DupUoMs uoms ({
		// Time:
		{ "ns", { 1.0 / 1000 / 1000 / 1000, "seconds" } },
		{ "us", { 1.0 / 1000 / 1000, "seconds" } },
		{ "ms", { 1.0 / 1000, "seconds" } },
		{ "s", { 1, "seconds" } },
		{ "m", { 60, "seconds" } },
		{ "h", { 60 * 60, "seconds" } },
		{ "d", { 60 * 60 * 24, "seconds" } },

		// Mass:
		{ "ng", { 1.0 / 1000 / 1000 / 1000, "grams" } },
		{ "ug", { 1.0 / 1000 / 1000, "grams" } },
		{ "mg", { 1.0 / 1000, "grams" } },
		{ "g", { 1, "grams" } },
		{ "kg", { 1000, "grams" } },
		{ "t", { 1000 * 1000, "grams" } },

		// Volume:
		{ "ml", { 1.0 / 1000, "liters" } },
		{ "l", { 1, "liters" } },
		{ "hl", { 100, "liters" } },

		// Misc:
		{ "packets", { 1, "packets" } },
		{ "lm", { 1, "lumens" } },
		{ "dbm", { 1, "decibel-milliwatts" } },
		{ "f", { 1, "degrees-fahrenheit" } },
		{ "k", { 1, "degrees-kelvin" } }
	});

	UoMs uniqUoms;

	for (auto& uom : uoms) {
		if (!uniqUoms.emplace(uom).second) {
			throw std::logic_error("Duplicate case-insensitive UoM detected: " + uom.first);
		}
	}

	for (auto& uom : l_CsUoMs) {
		auto input (uom.first);
		boost::algorithm::to_lower(input);

		auto pos (uoms.find(input));

		if (pos != uoms.end()) {
			throw std::logic_error("Duplicate case-sensitive/case-insensitive UoM detected: " + pos->first);
		}
	}

	return uniqUoms;
})());

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
	std::vector<String> tokens = valueStr.Split(";");

	if (valueStr.FindFirstOf(',') != String::NPos || tokens.empty()) {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid performance data value: " + perfdata));
	}

	// Find the position where to split value and unit. Possible values of tokens[0] include:
	// "1000", "1.0", "1.", "-.1", "+1", "1e10", "1GB", "1e10GB", "1e10EB", "1E10EB", "1.5GB", "1.GB", "+1.E-1EW"
	// Consider everything up to and including the last digit or decimal point as part of the value.
	size_t pos = tokens[0].FindLastOf("0123456789.");
	if (pos != String::NPos) {
		pos++;
	}

	double value = Convert::ToDouble(tokens[0].SubStr(0, pos));

	if (!std::isfinite(value)) {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid performance data value: " + perfdata + " is outside of any reasonable range"));
	}

	bool counter = false;
	String unit;
	Value warn, crit, min, max;

	if (pos != String::NPos)
		unit = tokens[0].SubStr(pos, String::NPos);

	// UoM.Out is an empty string for "c". So set counter before parsing.
	if (unit == "c") {
		counter = true;
	}

	double base;

	{
		auto uom (l_CsUoMs.find(unit.GetData()));

		if (uom == l_CsUoMs.end()) {
			auto ciUnit (unit.ToLower());
			auto uom (l_CiUoMs.find(ciUnit.GetData()));

			if (uom == l_CiUoMs.end()) {
				Log(LogDebug, "PerfdataValue")
					<< "Invalid performance data unit: " << unit;

				unit = "";
				base = 1.0;
			} else {
				unit = uom->second.Out;
				base = uom->second.Factor;
			}
		} else {
			unit = uom->second.Out;
			base = uom->second.Factor;
		}
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

static const std::unordered_map<std::string, const char*> l_FormatUoMs ({
	{ "ampere-seconds", "As" },
	{ "amperes", "A" },
	{ "bits", "b" },
	{ "bytes", "B" },
	{ "decibel-milliwatts", "dBm" },
	{ "degrees-celsius", "C" },
	{ "degrees-fahrenheit", "F" },
	{ "degrees-kelvin", "K" },
	{ "grams", "g" },
	{ "liters", "l" },
	{ "lumens", "lm" },
	{ "ohms", "O" },
	{ "percent", "%" },
	{ "seconds", "s" },
	{ "volts", "V" },
	{ "watt-hours", "Wh" },
	{ "watts", "W" }
});

String PerfdataValue::Format() const
{
	std::ostringstream result;

	if (GetLabel().FindFirstOf(" ") != String::NPos)
		result << "'" << GetLabel() << "'";
	else
		result << GetLabel();

	result << "=" << Convert::ToString(GetValue());

	String unit;

	if (GetCounter()) {
		unit = "c";
	} else {
		auto myUnit (GetUnit());
		auto uom (l_FormatUoMs.find(myUnit.GetData()));

		if (uom != l_FormatUoMs.end()) {
			unit = uom->second;
		}
	}

	result << unit;

	std::string interm(";");
	if (!GetWarn().IsEmpty()) {
		result << interm << Convert::ToString(GetWarn());
		interm.clear();
	}

	interm += ";";
	if (!GetCrit().IsEmpty()) {
		result << interm << Convert::ToString(GetCrit());
		interm.clear();
	}

	interm += ";";
	if (!GetMin().IsEmpty()) {
		result << interm << Convert::ToString(GetMin());
		interm.clear();
	}

	interm += ";";
	if (!GetMax().IsEmpty()) {
		result << interm << Convert::ToString(GetMax());
	}

	return result.str();
}

Value PerfdataValue::ParseWarnCritMinMaxToken(const std::vector<String>& tokens, std::vector<String>::size_type index, const String& description)
{
	if (tokens.size() > index && tokens[index] != "U" && tokens[index] != "" && tokens[index].FindFirstNotOf("+-0123456789.eE") == String::NPos)
		return Convert::ToDouble(tokens[index]);
	else {
		if (tokens.size() > index && tokens[index] != "")
			Log(LogDebug, "PerfdataValue")
				<< "Ignoring unsupported perfdata " << description << " range, value: '" << tokens[index] << "'.";
		return Empty;
	}
}
