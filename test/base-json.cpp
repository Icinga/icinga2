/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/namespace.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_json)

BOOST_AUTO_TEST_CASE(encode)
{
	Dictionary::Ptr input (new Dictionary({
		{ "array", new Array({ new Namespace() }) },
		{ "false", false },
		{ "float", -1.25 },
		{ "fx", new Function("<test>", []() {}) },
		{ "int", -42 },
		{ "null", Value() },
		{ "string", "LF\nTAB\tAUml\xC3\xA4Ill\xC3" },
		{ "true", true },
		{ "uint", 23u }
	}));

	String output (R"EOF({
    "array": [
        {}
    ],
    "false": false,
    "float": -1.25,
    "fx": "Object of type 'Function'",
    "int": -42,
    "null": null,
    "string": "LF\nTAB\tAUml\u00e4Ill\ufffd",
    "true": true,
    "uint": 23
}
)EOF");

	BOOST_CHECK(JsonEncode(input, true) == output);

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

static std::string MakeNestedJsonArray(size_t depth)
{
	return std::string(depth, '[') + std::string(depth, ']');
}

static std::string MakeNestedJsonObject(size_t depth)
{
	std::ostringstream buf;
	for (size_t i = 0; i < depth; ++i) {
		buf << "{\"" << i << "\":";
	}
	buf << "null";
	for (size_t i = 0; i < depth; ++i) {
		buf << "}";
	}
	return buf.str();
}

BOOST_AUTO_TEST_CASE(decode_depth_limit)
{
	auto isDepthLimit = [](const std::exception& ex) {
		return std::string_view(ex.what()) == "JSON decoding recursion limit reached";
	};

	// Scalars parse even with depth limit 0.
	BOOST_CHECK_EQUAL(JsonDecode("42", 0), Value(42));
	BOOST_CHECK_EQUAL(JsonDecode("true", 0), Value(true));
	BOOST_CHECK_EQUAL(JsonDecode("\"test\"", 0), Value("test"));

	// Arrays and objects require at least depth limit 1.
	BOOST_CHECK_EXCEPTION(JsonDecode("[]", 0), std::exception, isDepthLimit);
	BOOST_CHECK_EXCEPTION(JsonDecode("{}", 0), std::exception, isDepthLimit);
	BOOST_CHECK_EXCEPTION(JsonDecode("[42]", 0), std::exception, isDepthLimit);
	BOOST_CHECK_EXCEPTION(JsonDecode("{\"foo\": 23}", 0), std::exception, isDepthLimit);

	// Array in array and object in object require at least depth limit 2.
	BOOST_CHECK_EXCEPTION(JsonDecode("[[]]", 1), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode("[[]]", 2));
	BOOST_CHECK_EXCEPTION(JsonDecode(R"({"a":{}})", 1), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode(R"({"a":{}})", 2));

	// Mixed nesting of arrays and objects is both counted towards the same limit.
	BOOST_CHECK_EXCEPTION(JsonDecode("[{}]", 1), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode("[{}]", 2));
	BOOST_CHECK_EXCEPTION(JsonDecode(R"({"a":[]})", 1), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode(R"({"a":[]})", 2));

	// Siblings are not added up for the depth check.
	BOOST_CHECK_NO_THROW(JsonDecode("[[], [], [], {}, [], [], {}, {}, [], {}, {}, {}]", 2));
	BOOST_CHECK_NO_THROW(JsonDecode(R"({"a": [], "b": {}, "c": {}, "d": [], "e": [], "f": {}})", 2));

	// Some deeper nested array.
	std::string arrayWithNesting42 = MakeNestedJsonArray(42);
	BOOST_CHECK_EXCEPTION(JsonDecode(arrayWithNesting42, 41), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode(arrayWithNesting42, 42));

	// Some deeper nested object.
	std::string objectWithNesting42 = MakeNestedJsonObject(42);
	BOOST_CHECK_EXCEPTION(JsonDecode(objectWithNesting42, 41), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode(objectWithNesting42, 42));

	// And some deeper nested mixed containers.
	std::string deeperMixedNesting = R"({
		"1st-level": [
			true,
			"2nd-level",
			{
				"dummy": 42,
				"3rd-level": {
					"4th-level": [
						"5th-level",
						{
							"6th-level": {
								"7th-level": {
									"8th-level": [
										"9th-level",
										[
											"10th-level"
										]
									]
								}
							}
						}
					]
				}
			}
		]
	})";
	BOOST_CHECK_EXCEPTION(JsonDecode(deeperMixedNesting, 9), std::exception, isDepthLimit);
	BOOST_CHECK_NO_THROW(JsonDecode(deeperMixedNesting, 10));
}

BOOST_AUTO_TEST_SUITE_END()
