/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/perfdatavalue.h"
#include "base/dictionary.h"
#include "base/objectlock.h"
#include "base/application.h"
#include "base/type.h"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_type)

BOOST_AUTO_TEST_CASE(gettype)
{
	const Type *t = Type::GetByName("Application");

	BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(assign)
{
	const Type *t1 = Type::GetByName("Application");
	const Type *t2 = Type::GetByName("DynamicObject");

	BOOST_CHECK(t1->IsAssignableFrom(t1));
	BOOST_CHECK(t2->IsAssignableFrom(t1));
	BOOST_CHECK(!t1->IsAssignableFrom(t2));
}

BOOST_AUTO_TEST_CASE(byname)
{
	const Type *t = Type::GetByName("Application");

	BOOST_CHECK(t);
}

BOOST_AUTO_TEST_CASE(instantiate)
{
	const Type *t = Type::GetByName("PerfdataValue");

	Object::Ptr p = t->Instantiate();

	BOOST_CHECK(p);
}

BOOST_AUTO_TEST_SUITE_END()
