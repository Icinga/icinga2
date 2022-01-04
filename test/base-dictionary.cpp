/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/string.hpp"
#include "base/utility.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_dictionary)

BOOST_AUTO_TEST_CASE(construct)
{
	Dictionary::Ptr dictionary = new Dictionary();
	BOOST_CHECK(dictionary);
}

BOOST_AUTO_TEST_CASE(initializer1)
{
	DictionaryData dict;

	dict.emplace_back("test1", "Gin-o-clock");

	Dictionary::Ptr dictionary = new Dictionary(std::move(dict));

	Value test1;
	test1 = dictionary->Get("test1");
	BOOST_CHECK(test1 == "Gin-o-clock");
}

BOOST_AUTO_TEST_CASE(initializer2)
{
	Dictionary::Ptr dictionary = new Dictionary({ {"test1", "Gin-for-the-win"} });

	Value test1;
	test1 = dictionary->Get("test1");
	BOOST_CHECK(test1 == "Gin-for-the-win");
}

BOOST_AUTO_TEST_CASE(get1)
{
	Dictionary::Ptr dictionary = new Dictionary();
	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	BOOST_CHECK(dictionary->GetLength() == 2);

	Value test1;
	test1 = dictionary->Get("test1");
	BOOST_CHECK(test1 == 7);

	Value test2;
	test2 = dictionary->Get("test2");
	BOOST_CHECK(test2 == "hello world");

	String key3 = "test3";
	Value test3;
	test3 = dictionary->Get(key3);
	BOOST_CHECK(test3.IsEmpty());
}

BOOST_AUTO_TEST_CASE(get2)
{
	Dictionary::Ptr dictionary = new Dictionary();
	Dictionary::Ptr other = new Dictionary();

	dictionary->Set("test1", other);

	BOOST_CHECK(dictionary->GetLength() == 1);

	Dictionary::Ptr test1 = dictionary->Get("test1");
	BOOST_CHECK(other == test1);

	Dictionary::Ptr test2 = dictionary->Get("test2");
	BOOST_CHECK(!test2);
}

BOOST_AUTO_TEST_CASE(foreach)
{
	Dictionary::Ptr dictionary = new Dictionary();
	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	ObjectLock olock(dictionary);

	bool seen_test1 = false, seen_test2 = false;

	for (const auto& kv : dictionary) {
		BOOST_CHECK(kv.first == "test1" || kv.first == "test2");

		if (kv.first == "test1") {
			BOOST_CHECK(!seen_test1);
			seen_test1 = true;

			BOOST_CHECK(kv.second == 7);

			continue;
		} else if (kv.first == "test2") {
			BOOST_CHECK(!seen_test2);
			seen_test2 = true;

			BOOST_CHECK(kv.second == "hello world");
		}
	}

	BOOST_CHECK(seen_test1);
	BOOST_CHECK(seen_test2);
}

BOOST_AUTO_TEST_CASE(remove)
{
	Dictionary::Ptr dictionary = new Dictionary();

	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	BOOST_CHECK(dictionary->Contains("test1"));
	BOOST_CHECK(dictionary->GetLength() == 2);

	dictionary->Set("test1", Empty);

	BOOST_CHECK(dictionary->Contains("test1"));
	BOOST_CHECK(dictionary->GetLength() == 2);

	dictionary->Remove("test1");

	BOOST_CHECK(!dictionary->Contains("test1"));
	BOOST_CHECK(dictionary->GetLength() == 1);

	dictionary->Remove("test2");

	BOOST_CHECK(!dictionary->Contains("test2"));
	BOOST_CHECK(dictionary->GetLength() == 0);

	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	{
		ObjectLock olock(dictionary);

		auto it = dictionary->Begin();
		dictionary->Remove(it);
	}

	BOOST_CHECK(dictionary->GetLength() == 1);
}

BOOST_AUTO_TEST_CASE(clone)
{
	Dictionary::Ptr dictionary = new Dictionary();

	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	Dictionary::Ptr clone = dictionary->ShallowClone();

	BOOST_CHECK(dictionary != clone);

	BOOST_CHECK(clone->GetLength() == 2);
	BOOST_CHECK(clone->Get("test1") == 7);
	BOOST_CHECK(clone->Get("test2") == "hello world");

	clone->Set("test3", 5);
	BOOST_CHECK(!dictionary->Contains("test3"));
	BOOST_CHECK(dictionary->GetLength() == 2);

	clone->Set("test2", "test");
	BOOST_CHECK(dictionary->Get("test2") == "hello world");
}

BOOST_AUTO_TEST_CASE(json)
{
	Dictionary::Ptr dictionary = new Dictionary();

	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	String json = JsonEncode(dictionary);
	BOOST_CHECK(json.GetLength() > 0);
	Dictionary::Ptr deserialized = JsonDecode(json);
	BOOST_CHECK(deserialized->GetLength() == 2);
	BOOST_CHECK(deserialized->Get("test1") == 7);
	BOOST_CHECK(deserialized->Get("test2") == "hello world");
}

BOOST_AUTO_TEST_CASE(keys_ordered)
{
	Dictionary::Ptr dictionary = new Dictionary();

	for (int i = 0; i < 100; i++) {
		dictionary->Set(std::to_string(Utility::Random()), Utility::Random());
	}

	std::vector<String> keys = dictionary->GetKeys();
	BOOST_CHECK(std::is_sorted(keys.begin(), keys.end()));
}

BOOST_AUTO_TEST_SUITE_END()
