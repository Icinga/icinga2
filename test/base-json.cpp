// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/convert.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/namespace.hpp"
#include "base/array.hpp"
#include "base/generator.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "test/utils.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <BoostTestTargetConfig.h>
#include <limits>
#include <cmath>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_json)

BOOST_AUTO_TEST_CASE(encode)
{
	int emptyGenCounter = 0;
	std::vector<int> empty;
	std::vector<int> vec{1, 2, 3};
	auto generate = [](int count) -> Value { return Value(count); };

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
		{ "generator", new ValueGenerator{vec, generate} },
		{
			"empty_generator",
			new ValueGenerator{
				empty,
				[&emptyGenCounter](int) -> Value {
					emptyGenCounter++;
					return Empty;
				}
			}
		},
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
	BOOST_CHECK_EQUAL(emptyGenCounter, 0); // Ensure the transformation function was never invoked.
	input->Set("generator", new ValueGenerator{vec, generate});

	std::ostringstream oss;
	JsonEncode(input, oss, true);
	BOOST_CHECK_EQUAL(emptyGenCounter, 0); // Ensure the transformation function was never invoked.
	BOOST_CHECK_EQUAL(output, oss.str());

	boost::algorithm::replace_all(output, " ", "");
	boost::algorithm::replace_all(output, "Objectoftype'Function'", "Object of type 'Function'");
	boost::algorithm::replace_all(output, "\n", "");

	input->Set("generator", new ValueGenerator{vec, generate});
	BOOST_CHECK(JsonEncode(input, false) == output);
	BOOST_CHECK_EQUAL(emptyGenCounter, 0); // Ensure the transformation function was never invoked.
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
	auto ExpectLimitExceeded = [](std::string_view input, size_t limit, std::string_view path) {
		try {
			JsonDecode(std::string(input), limit);

			boost::test_tools::assertion_result result{false};
			result.message() << "Decoding '" << input << "' with limit " << limit << " did not throw an exception";
			return result;
		} catch (const std::exception& ex) {
			std::ostringstream expected;
			expected << "JSON decoding recursion limit reached (path: " << path << ")";

			boost::test_tools::assertion_result result{ex.what() == expected.str()};
			result.message() << "Decoding '" << input << "' with limit " << limit << ":\n"
				<< "      got exception: " << ex.what() << "\n"
				<< " expected exception: " << expected.str() << "\n";
			return result;
		}
	};

	// Scalars parse even with depth limit 0.
	BOOST_CHECK_EQUAL(JsonDecode("42", 0), Value(42));
	BOOST_CHECK_EQUAL(JsonDecode("true", 0), Value(true));
	BOOST_CHECK_EQUAL(JsonDecode("\"test\"", 0), Value("test"));

	// Arrays and objects require at least depth limit 1.
	BOOST_CHECK(ExpectLimitExceeded("[]", 0, "root"));
	BOOST_CHECK(ExpectLimitExceeded("{}", 0, "root"));
	BOOST_CHECK(ExpectLimitExceeded("[42]", 0, "root"));
	BOOST_CHECK(ExpectLimitExceeded("{\"foo\": 23}", 0, "root"));

	// Array in array and object in object require at least depth limit 2.
	BOOST_CHECK(ExpectLimitExceeded("[[]]", 1, "root[0]"));
	BOOST_CHECK_NO_THROW(JsonDecode("[[]]", 2));
	BOOST_CHECK(ExpectLimitExceeded(R"({"a":{}})", 1, R"(root["a"])"));
	BOOST_CHECK_NO_THROW(JsonDecode(R"({"a":{}})", 2));

	// Mixed nesting of arrays and objects is both counted towards the same limit.
	BOOST_CHECK(ExpectLimitExceeded("[{}]", 1, "root[0]"));
	BOOST_CHECK_NO_THROW(JsonDecode("[{}]", 2));
	BOOST_CHECK(ExpectLimitExceeded(R"({"a":[]})", 1, R"(root["a"])"));
	BOOST_CHECK_NO_THROW(JsonDecode(R"({"a":[]})", 2));

	// Siblings are not added up for the depth check.
	BOOST_CHECK_NO_THROW(JsonDecode("[[], [], [], {}, [], [], {}, {}, [], {}, {}, {}]", 2));
	BOOST_CHECK_NO_THROW(JsonDecode(R"({"a": [], "b": {}, "c": {}, "d": [], "e": [], "f": {}})", 2));

	// Some deeper nested array.
	std::string arrayWithNesting42 = MakeNestedJsonArray(42);
	std::ostringstream arrayPathWithNesting41;
	arrayPathWithNesting41 << "root";
	for (size_t i = 0; i < 41; ++i) {
		arrayPathWithNesting41 << "[0]";
	}
	BOOST_CHECK(ExpectLimitExceeded(arrayWithNesting42, 41, arrayPathWithNesting41.str()));
	BOOST_CHECK_NO_THROW(JsonDecode(arrayWithNesting42, 42));

	// Some deeper nested object.
	std::string objectWithNesting42 = MakeNestedJsonObject(42);
	std::ostringstream objectPathWithNesting41;
	objectPathWithNesting41 << "root";
	for (size_t i = 0; i < 41; ++i) {
		objectPathWithNesting41 << "[\"" << i << "\"]";
	}
	BOOST_CHECK(ExpectLimitExceeded(objectWithNesting42, 41, objectPathWithNesting41.str()));
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
	auto deeperPath = R"(root["1st-level"][2]["3rd-level"]["4th-level"][1]["6th-level"]["7th-level"]["8th-level"][1])";
	BOOST_CHECK(ExpectLimitExceeded(deeperMixedNesting, 9, deeperPath));
	BOOST_CHECK_NO_THROW(JsonDecode(deeperMixedNesting, 10));
}

/* This test case decodes JSON nested much deeper (see safetyFactor) than the default depth limit and performs some
 * operations on the resulting values. This is done within a coroutine with its limited stack size on order to verify
 * that the default limit is low enough to be safe. Note that this isn't an exact science unfortunately: other
 * recursive operations may have larger stack frames and these operations aren't the only thing on a stack (could be
 * done inside an HTTP or JSON-RPC connection for example). Therefor, aim for a sufficiently large safety factor.
 */
BOOST_AUTO_TEST_CASE(coroutine_stack_size)
{
	auto future = SpawnSynchronizedCoroutine([](boost::asio::yield_context) {
		constexpr size_t safetyFactor = 10;
		constexpr size_t depth = JsonDecodeDefaultDepthLimit * safetyFactor;

		Value val;

		BOOST_REQUIRE_NO_THROW(val = JsonDecode(MakeNestedJsonArray(depth), depth));
		BOOST_REQUIRE_NO_THROW(val.Clone());
		BOOST_REQUIRE_NO_THROW(Convert::ToString(val));
		BOOST_REQUIRE_NO_THROW(JsonDecode(JsonEncode(val), depth));

		BOOST_REQUIRE_NO_THROW(val = JsonDecode(MakeNestedJsonObject(depth), depth));
		BOOST_REQUIRE_NO_THROW(val.Clone());
		BOOST_REQUIRE_NO_THROW(Convert::ToString(val));
		BOOST_REQUIRE_NO_THROW(JsonDecode(JsonEncode(val), depth));
	});
	future.get();
}

BOOST_AUTO_TEST_SUITE_END()
