/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/function.hpp"
#include "base/scriptframe.hpp"
#include "base/exception.hpp"
#include "config/configcompiler.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

static bool SideEffectFreeLess(const Value& lhs, const Value& rhs)
{
	return lhs < rhs;
}

BOOST_AUTO_TEST_SUITE(base_array)

BOOST_AUTO_TEST_CASE(construct)
{
	Array::Ptr array = new Array();
	BOOST_CHECK(array);
	BOOST_CHECK(array->GetLength() == 0);
}

BOOST_AUTO_TEST_CASE(getset)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);
	BOOST_CHECK(array->GetLength() == 3);
	BOOST_CHECK(array->Get(0) == 7);
	BOOST_CHECK(array->Get(1) == 2);
	BOOST_CHECK(array->Get(2) == 5);

	array->Set(1, 9);
	BOOST_CHECK(array->Get(1) == 9);

	array->Remove(1);
	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(1) == 5);
}

BOOST_AUTO_TEST_CASE(resize)
{
	Array::Ptr array = new Array();
	array->Resize(2);
	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(0) == Empty);
	BOOST_CHECK(array->Get(1) == Empty);
}

BOOST_AUTO_TEST_CASE(insert)
{
	Array::Ptr array = new Array();

	array->Insert(0, 11);
	array->Insert(1, 22);
	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(1) == 22);

	array->Insert(0, 33);
	BOOST_CHECK(array->GetLength() == 3);
	BOOST_CHECK(array->Get(0) == 33);
	BOOST_CHECK(array->Get(1) == 11);

	array->Insert(1, 44);
	BOOST_CHECK(array->GetLength() == 4);
	BOOST_CHECK(array->Get(0) == 33);
	BOOST_CHECK(array->Get(1) == 44);
	BOOST_CHECK(array->Get(2) == 11);
}

BOOST_AUTO_TEST_CASE(remove)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	{
		ObjectLock olock(array);
		auto it = array->Begin();
		array->Remove(it);
	}

	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(0) == 2);

	array->Clear();
	BOOST_CHECK(array->GetLength() == 0);
}

BOOST_AUTO_TEST_CASE(unique)
{
	Array::Ptr array = new Array();
	array->Add("group1");
	array->Add("group2");
	array->Add("group1");
	array->Add("group2");

	Array::Ptr result;

	{
		ObjectLock olock(array);
		result = array->Unique();
	}

	BOOST_CHECK(result->GetLength() == 2);
	result->Sort();

	BOOST_CHECK(result->Get(0) == "group1");
	BOOST_CHECK(result->Get(1) == "group2");
}

BOOST_AUTO_TEST_CASE(sort_script)
{
	ScriptFrame frame (true);

	auto Eval = [&frame](const String& text) {
		return ConfigCompiler::CompileText("<test>", text)->Evaluate(frame).GetValue();
	};

	/* The default comparator and an explicit comparator that replicates it must behave
	 * identically, so run these checks against both instead of duplicating them. */
	for (const char* fnStr : {"", "function(a, b) { return a < b }"}) {
		BOOST_TEST_CONTEXT("With function param: " << fnStr) {
			/* Empty and single-element arrays. */
			{
				Array::Ptr arr = Eval(String("[].sort(") + fnStr + ")");
				std::vector<Value> expected;
				ObjectLock olock (arr);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
			}

			{
				Array::Ptr arr = Eval(String("[42].sort(") + fnStr + ")");
				std::vector<Value> expected {42};
				ObjectLock olock (arr);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
			}

			/* Ascending sort. */
			{
				Array::Ptr arr = Eval(String("[2, 1].sort(") + fnStr + ")");
				std::vector<Value> expected{ 1, 2};
				ObjectLock olock (arr);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
			}

			{
				Array::Ptr arr = Eval(String("[3, 1, 2].sort(") + fnStr + ")");
				std::vector<Value> expected {1, 2, 3};
				ObjectLock olock (arr);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
			}

			/* Duplicate elements. */
			{
				Array::Ptr arr = Eval(String("[2, 1, 2, 1].sort(") + fnStr + ")");
				std::vector<Value> expected {1, 1, 2, 2};
				ObjectLock olock (arr);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
			}

			/* sort() clones: the original array is left untouched. */
			{
				frame.Locals->Set("original", new Array{3, 1, 2});
				Array::Ptr sorted = Eval(String("original.sort(") + fnStr + ")");
				Array::Ptr original = frame.Locals->Get("original");

				std::vector<Value> expectedSorted {1, 2, 3};
				std::vector<Value> expectedOriginal {3, 1, 2};

				ObjectLock slock (sorted);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(sorted), end(sorted), expectedSorted.begin(), expectedSorted.end());

				ObjectLock olock (original);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(original), end(original), expectedOriginal.begin(), expectedOriginal.end());
			}

			/* A frozen source array can still be sorted since sort() clones before sorting. */
			{
				frame.Locals->Set("frozen", new Array{3, 1, 2});
				Eval("frozen.freeze()");
				Array::Ptr arr = Eval(String("frozen.sort(") + fnStr + ")");
				std::vector<Value> expected{1, 2, 3};
				ObjectLock olock (arr);
				BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
			}

			/* Mixing incomparable types throws regardless of the comparator used. */
			BOOST_CHECK_THROW(Eval(String("[1, \"a\"].sort(") + fnStr + ")"), ScriptError);
		}
	}

	/* Custom comparator, descending: not expressible via the default comparator. */
	{
		Array::Ptr arr = Eval("[3, 1, 2].sort(function(a, b) { return a > b })");
		std::vector<Value> expected{3, 2, 1};
		ObjectLock olock (arr);
		BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
	}

	/* A null/non-function comparator throws. */
	BOOST_CHECK_THROW(Eval("[3, 1, 2].sort(null)"), ScriptError);

	/* An exception raised by the comparator propagates. */
	BOOST_CHECK_THROW(Eval("[3, 1, 2].sort(function(a, b) { throw \"boom\" })"), ScriptError);

	/* Sandboxed frame: a comparator that is not side-effect-free is rejected. */
	{
		frame.Sandboxed = true;
		BOOST_CHECK_THROW(Eval("[3, 1, 2].sort(function(a, b) { return a < b })"), ScriptError);
	}

	/* Sandboxed frame: a side-effect-free comparator is allowed. */
	{
		frame.Locals->Set("cmp", new Function("less", SideEffectFreeLess, { "a", "b" }, true));
		Array::Ptr arr = Eval("[3, 1, 2].sort(cmp)");

		std::vector<Value> expected {1, 2, 3};
		ObjectLock olock (arr);
		BOOST_CHECK_EQUAL_COLLECTIONS(begin(arr), end(arr), expected.begin(), expected.end());
	}
}

BOOST_AUTO_TEST_CASE(foreach)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	ObjectLock olock(array);

	int n = 0;

	for (const Value& item : array) {
		BOOST_CHECK(n != 0 || item == 7);
		BOOST_CHECK(n != 1 || item == 2);
		BOOST_CHECK(n != 2 || item == 5);

		n++;
	}
}

BOOST_AUTO_TEST_CASE(clone)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	Array::Ptr clone = array->ShallowClone();

	BOOST_CHECK(clone->GetLength() == 3);
	BOOST_CHECK(clone->Get(0) == 7);
	BOOST_CHECK(clone->Get(1) == 2);
	BOOST_CHECK(clone->Get(2) == 5);
}

BOOST_AUTO_TEST_CASE(json)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	String json = JsonEncode(array);
	BOOST_CHECK(json.GetLength() > 0);

	Array::Ptr deserialized = JsonDecode(json);
	BOOST_CHECK(deserialized);
	BOOST_CHECK(deserialized->GetLength() == 3);
	BOOST_CHECK(deserialized->Get(0) == 7);
	BOOST_CHECK(deserialized->Get(1) == 2);
	BOOST_CHECK(deserialized->Get(2) == 5);
}

BOOST_AUTO_TEST_SUITE_END()
