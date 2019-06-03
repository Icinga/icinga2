/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/perfdatavalue.hpp"
#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/application.hpp"
#include "base/type.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_type)

BOOST_AUTO_TEST_CASE(gettype)
{
	Type::Ptr t = Type::GetByName("Application");

	BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(assign)
{
	Type::Ptr t1 = Type::GetByName("Application");
	Type::Ptr t2 = Type::GetByName("ConfigObject");

	BOOST_CHECK(t1->IsAssignableFrom(t1));
	BOOST_CHECK(t2->IsAssignableFrom(t1));
	BOOST_CHECK(!t1->IsAssignableFrom(t2));
}

BOOST_AUTO_TEST_CASE(byname)
{
	Type::Ptr t = Type::GetByName("Application");

	BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(instantiate)
{
	Type::Ptr t = Type::GetByName("PerfdataValue");

	Object::Ptr p = t->Instantiate(std::vector<Value>());

	BOOST_CHECK(p);
}

BOOST_AUTO_TEST_SUITE_END()
