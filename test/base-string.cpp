/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/string.hpp"
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

BOOST_AUTO_TEST_SUITE_END()
