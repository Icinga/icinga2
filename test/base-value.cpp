/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/value.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_value)

BOOST_AUTO_TEST_CASE(scalar)
{
	Value v;

	v = 3;
	BOOST_CHECK(v.IsScalar());

	v = "hello";
	BOOST_CHECK(v.IsScalar());

	v = Empty;
	BOOST_CHECK(!v.IsScalar());
}

BOOST_AUTO_TEST_CASE(convert)
{
	Value v;
	BOOST_CHECK(v.IsEmpty());
	BOOST_CHECK(v == "");
	BOOST_CHECK(static_cast<double>(v) == 0);
	BOOST_CHECK(!v.IsScalar());
	BOOST_CHECK(!v.IsObjectType<Object>());

	BOOST_CHECK(v + "hello" == "hello");
	BOOST_CHECK("hello" + v == "hello");
}

BOOST_AUTO_TEST_CASE(format)
{
	Value v = 3;

	std::ostringstream obuf;
	obuf << v;

	BOOST_CHECK(obuf.str() == "3");

	std::istringstream ibuf("3");
	ibuf >> v;

	BOOST_CHECK_MESSAGE(v.IsString(), "type of v should be String (is " << v.GetTypeName() << ")");
	BOOST_CHECK_MESSAGE(v == "3", "v should be '3' (is '" << v << "')");
}

BOOST_AUTO_TEST_SUITE_END()
