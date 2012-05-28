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

BOOST_AUTO_TEST_CASE(setproperty)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();
	dictionary->SetProperty("test1", 7);
	dictionary->SetProperty("test2", "hello world");

	long test1;
	BOOST_REQUIRE(dictionary->GetProperty("test1", &test1));
	BOOST_REQUIRE(test1 == 7);

	string test2;
	BOOST_REQUIRE(dictionary->GetProperty("test2", &test2));
	BOOST_REQUIRE(test2 == "hello world");
}
