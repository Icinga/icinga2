/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/namespace.hpp"
#include "base/array.hpp"
#include "base/generator.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <BoostTestTargetConfig.h>
#include <limits>
#include <cmath>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_json)

BOOST_AUTO_TEST_CASE(encode)
{
	auto generate = []() -> std::optional<Value> {
		static int count = 0;
		if (++count == 4) {
			count = 0;
			return std::nullopt;
		}
		return Value(count);
	};

	Dictionary::Ptr input (new Dictionary({
		{ "array", new Array({ new Namespace() }) },
		{ "false", false },
		// Use double max value to test JSON encoding of large numbers and trigger boost numeric_cast exceptions
		{ "max_double", std::numeric_limits<double>::max() },
		// Test the largest uint64_t value that has an exact double representation (2^64-2048).
		{ "max_int_in_double", std::nextafter(std::pow(2, 64), 0.0) },
		{ "float", -1.25f },
		{ "float_without_fraction", 23.0f },
		{ "fx", new Function("<test>", []() {}) },
		{ "int", -42 },
		{ "null", Value() },
		{ "string", "LF\nTAB\tAUml\xC3\xA4Ill\xC3" },
		{ "true", true },
		{ "uint", 23u },
		{ "generator", new ValueGenerator(generate) },
		{ "empty_generator", new ValueGenerator([]() -> std::optional<Value> { return std::nullopt; }) },
	}));

	String output (R"EOF({
    "array": [
        {}
    ],
    "empty_generator": [],
    "false": false,
    "float": -1.25,
    "float_without_fraction": 23,
    "fx": "Object of type 'Function'",
    "generator": [
        1,
        2,
        3
    ],
    "int": -42,
    "max_double": 1.7976931348623157e+308,
    "max_int_in_double": 18446744073709549568,
    "null": null,
    "string": "LF\nTAB\tAUml\u00e4Ill\ufffd",
    "true": true,
    "uint": 23
}
)EOF");

	auto got(JsonEncode(input, true));
	BOOST_CHECK_EQUAL(output, got);

	std::ostringstream oss;
	JsonEncode(input, oss, true);
	BOOST_CHECK_EQUAL(output, oss.str());

	boost::algorithm::replace_all(output, " ", "");
	boost::algorithm::replace_all(output, "Objectoftype'Function'", "Object of type 'Function'");
	boost::algorithm::replace_all(output, "\n", "");

	BOOST_CHECK(JsonEncode(input, false) == output);
}

BOOST_AUTO_TEST_CASE(decode)
{
	String input (R"EOF({
    "array": [
        {}
    ],
    "false": false,
    "float": -1.25,
    "int": -42,
    "null": null,
    "string": "LF\nTAB\tAUmlIll",
    "true": true,
    "uint": 23
}
)EOF");

	boost::algorithm::replace_all(input, "AUml", "AUml\xC3\xA4");
	boost::algorithm::replace_all(input, "Ill", "Ill\xC3");

	auto output ((Dictionary::Ptr)JsonDecode(input));
	BOOST_CHECK(output->GetKeys() == std::vector<String>({"array", "false", "float", "int", "null", "string", "true", "uint"}));

	auto array ((Array::Ptr)output->Get("array"));
	BOOST_CHECK(array->GetLength() == 1u);

	auto array0 ((Dictionary::Ptr)array->Get(0));
	BOOST_CHECK(array0->GetKeys() == std::vector<String>());

	auto fAlse (output->Get("false"));
	BOOST_CHECK(fAlse.IsBoolean() && !fAlse.ToBool());

	auto fLoat (output->Get("float"));
	BOOST_CHECK(fLoat.IsNumber() && fLoat.Get<double>() == -1.25);

	auto iNt (output->Get("int"));
	BOOST_CHECK(iNt.IsNumber() && iNt.Get<double>() == -42.0);

	BOOST_CHECK(output->Get("null").IsEmpty());

	auto string (output->Get("string"));
	BOOST_CHECK(string.IsString() && string.Get<String>() == "LF\nTAB\tAUml\xC3\xA4Ill\xEF\xBF\xBD");

	auto tRue (output->Get("true"));
	BOOST_CHECK(tRue.IsBoolean() && tRue.ToBool());

	auto uint (output->Get("uint"));
	BOOST_CHECK(uint.IsNumber() && uint.Get<double>() == 23.0);
}

BOOST_AUTO_TEST_CASE(invalid1)
{
	BOOST_CHECK_THROW(JsonDecode("\"1.7"), std::exception);
	BOOST_CHECK_THROW(JsonDecode("{8: \"test\"}"), std::exception);
	BOOST_CHECK_THROW(JsonDecode("{\"test\": \"test\""), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
