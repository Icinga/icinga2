/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/object.hpp"
#include "base/value.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

class TestObject : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(TestObject);

	TestObject::Ptr GetTestRef()
	{
		return this;
	}
};

BOOST_AUTO_TEST_SUITE(base_object)

BOOST_AUTO_TEST_CASE(construct)
{
	Object::Ptr tobject = new TestObject();
	BOOST_CHECK(tobject);
}

BOOST_AUTO_TEST_CASE(getself)
{
	TestObject::Ptr tobject = new TestObject();
	TestObject::Ptr tobject_self = tobject->GetTestRef();
	BOOST_CHECK(tobject == tobject_self);

	Value vobject = tobject;
	BOOST_CHECK(!vobject.IsEmpty());
	BOOST_CHECK(vobject.IsObjectType<TestObject>());
}

BOOST_AUTO_TEST_SUITE_END()
