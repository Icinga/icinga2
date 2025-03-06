/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/string.hpp"
#include <vector>
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_string)

BOOST_AUTO_TEST_CASE(construct)
{
	BOOST_CHECK(String() == "");
	BOOST_CHECK(String(5, 'n') == "nnnnn");
}

BOOST_AUTO_TEST_CASE(equal)
{
	BOOST_CHECK(String("hello") == String("hello"));
	BOOST_CHECK("hello" == String("hello"));
	BOOST_CHECK(String("hello") == String("hello"));

	BOOST_CHECK(String("hello") != String("helloworld"));
	BOOST_CHECK("hello" != String("helloworld"));
	BOOST_CHECK(String("hello") != "helloworld");
}

BOOST_AUTO_TEST_CASE(clear)
{
	String s = "hello";
	s.Clear();
	BOOST_CHECK(s == "");
	BOOST_CHECK(s.IsEmpty());
}

BOOST_AUTO_TEST_CASE(append)
{
	String s;
	s += "he";
	s += String("ll");
	s += 'o';

	BOOST_CHECK(s == "hello");
}

BOOST_AUTO_TEST_CASE(trim)
{
	String s1 = "hello";
	BOOST_CHECK(s1.Trim() == "hello");

	String s2 = "  hello";
	BOOST_CHECK(s2.Trim() == "hello");

	String s3 = "hello ";
	BOOST_CHECK(s3.Trim() == "hello");

	String s4 = " hello ";
	BOOST_CHECK(s4.Trim() == "hello");
}

BOOST_AUTO_TEST_CASE(contains)
{
	String s1 = "hello world";
	String s2 = "hello";
	BOOST_CHECK(s1.Contains(s2));

	String s3 = "  hello world  ";
	String s4 = "  hello";
	BOOST_CHECK(s3.Contains(s4));

	String s5 = "  hello world  ";
	String s6 = "world  ";
	BOOST_CHECK(s5.Contains(s6));
}

BOOST_AUTO_TEST_CASE(replace)
{
	String s = "hello";

	s.Replace(0, 2, "x");
	BOOST_CHECK(s == "xllo");
}

BOOST_AUTO_TEST_CASE(index)
{
	String s = "hello";
	BOOST_CHECK(s[0] == 'h');

	s[0] = 'x';
	BOOST_CHECK(s == "xello");

	for (char& ch : s) {
		ch = 'y';
	}
	BOOST_CHECK(s == "yyyyy");
}

BOOST_AUTO_TEST_CASE(find)
{
	String s = "hello";
	BOOST_CHECK(s.Find("ll") == 2);
	BOOST_CHECK(s.FindFirstOf("xl") == 2);
}

// Check that if a std::vector<icinga::String> is grown beyond its capacity (i.e. it has to reallocate the memory),
// it uses the move constructor of icinga::String (i.e. the underlying string storage stays the same).
BOOST_AUTO_TEST_CASE(vector_move)
{
	std::vector<String> vec {
		// std::string (which is internally used by icinga::String) has an optimization that small strings can be
		// allocated inside it instead of in a separate heap allocation. In that case, the small string would still be
		// copied even by the move constructor. Using sizeof() ensures that the string is long enough so that it must
		// be allocated separately and can be used to test for the desired move to happen.
		std::string(sizeof(String) + 1, 'A'),
	};

	void *oldAddr = vec[0].GetData().data();
	// Sanity check that the data buffer is actually allocated outside the icinga::String instance.
	BOOST_CHECK(!(&vec[0] <= oldAddr && oldAddr < &vec[1]));

	// Force the vector to grow.
	vec.reserve(vec.capacity() + 1);

	// If the string was moved, the location of its underlying data buffer should not have changed.
	void *newAddr = vec[0].GetData().data();
	BOOST_CHECK_EQUAL(oldAddr, newAddr);
}

BOOST_AUTO_TEST_SUITE_END()
