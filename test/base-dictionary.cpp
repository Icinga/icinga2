#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE base_dictionary
#include <boost/test/unit_test.hpp>

#include <i2-base.h>
using namespace icinga;

BOOST_AUTO_TEST_CASE(construct)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();
	BOOST_REQUIRE(dictionary);
}

BOOST_AUTO_TEST_CASE(getproperty)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();
	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	long test1;
	BOOST_REQUIRE(dictionary->Get("test1", &test1));
	BOOST_REQUIRE(test1 == 7);

	string test2;
	BOOST_REQUIRE(dictionary->Get("test2", &test2));
	BOOST_REQUIRE(test2 == "hello world");

	long test3;
	BOOST_REQUIRE(!dictionary->Get("test3", &test3));
}

BOOST_AUTO_TEST_CASE(getproperty_dict)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();
	Dictionary::Ptr other = make_shared<Dictionary>();

	dictionary->Set("test1", other);

	Dictionary::Ptr test1;
	BOOST_REQUIRE(dictionary->Get("test1", &test1));
	BOOST_REQUIRE(other == test1);

	Dictionary::Ptr test2;
	BOOST_REQUIRE(!dictionary->Get("test2", &test2));
}

BOOST_AUTO_TEST_CASE(unnamed)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();
	dictionary->Add("test1");
	dictionary->Add("test2");
	dictionary->Add("test3");

	BOOST_REQUIRE(distance(dictionary->Begin(), dictionary->End()) == 3);
}
