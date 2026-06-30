/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/convert.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/namespace.hpp"
#include "base/array.hpp"
#include "base/io-engine.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/scriptglobal.hpp"
#include <functional>
#include <future>
#include <boost/algorithm/string/replace.hpp>
#include <BoostTestTargetConfig.h>

using namespace icinga;

static std::future<void> SpawnSynchronizedCoroutine(std::function<void(boost::asio::yield_context)> fn)
{
	using namespace icinga;

	auto promise = std::make_unique<std::promise<void>>();
	auto future = promise->get_future();
	auto& io = IoEngine::Get().GetIoContext();
	IoEngine::SpawnCoroutine(io, [promise = std::move(promise), fn = std::move(fn)](boost::asio::yield_context yc) {
		try {
			fn(std::move(yc));
		} catch (const std::exception&) {
			promise->set_exception(std::current_exception());
			return;
		}
		promise->set_value();
	});
	return future;
}

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

BOOST_AUTO_TEST_CASE(decode_dsl_single_argument)
{
	/* Regression test for #10913: the Json.decode() DSL function must accept a
	 * single argument. JsonDecode()'s optional depthLimit parameter must not
	 * leak into the function binding as a required argument.
	 */
	Namespace::Ptr systemNS = ScriptGlobal::Get("System");
	Namespace::Ptr jsonNS = systemNS->Get("Json");
	Function::Ptr decode = jsonNS->Get("decode");

	BOOST_REQUIRE(decode);
	BOOST_CHECK_EQUAL(decode->Invoke({ "2" }), Value(2));
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
 * operations on the resulting values. This is done with its limited stack size on order to verify that the default
 * limit is low enough to be safe. Note that this isn't an exact science unfortunately: other recursive operations may
 * have larger stack frames and these operations aren't the only thing on a stack (could be done inside an HTTP or
 * JSON-RPC connection for example). Therefor, aim for a sufficiently large safety factor. The test function may be
 * executed twice, once in a coroutine (which may not have a guard page and reliably detect an overflow depending on
 * the Boost version), once in a pthread thread (which may not be available everywhere).
 */
static void TestJsonStackSize()
{
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
}

BOOST_AUTO_TEST_CASE(stack_size_coroutine)
{
	auto future = SpawnSynchronizedCoroutine([](boost::asio::yield_context) {
		TestJsonStackSize();
	});
	future.get();
}

BOOST_AUTO_TEST_CASE(stack_size_pthread)
{
#ifdef HAVE_PTHREAD_CREATE
	auto pagesize = sysconf(_SC_PAGESIZE);
	BOOST_REQUIRE_GT(pagesize, 0);

	pthread_attr_t attr;
	BOOST_REQUIRE_EQUAL(0, pthread_attr_init(&attr));
	BOOST_REQUIRE_EQUAL(0, pthread_attr_setstacksize(&attr, IoEngine::GetCoroutineStackSize()));
	BOOST_REQUIRE_EQUAL(0, pthread_attr_setguardsize(&attr, pagesize));

	pthread_t thread;
	BOOST_REQUIRE_EQUAL(0, pthread_create(&thread, &attr, [](void*) -> void* {
		TestJsonStackSize();
		return nullptr;
	}, nullptr));

	BOOST_REQUIRE_EQUAL(0, pthread_join(thread, nullptr));
	BOOST_REQUIRE_EQUAL(0, pthread_attr_destroy(&attr));
#else /* HAVE_PTHREAD_CREATE */
	BOOST_TEST_MESSAGE("pthread not available");
#endif /* HAVE_PTHREAD_CREATE */
}

BOOST_AUTO_TEST_SUITE_END()
