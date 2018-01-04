/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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
