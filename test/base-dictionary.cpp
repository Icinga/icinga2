/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "base/dictionary.h"
#include <boost/test/unit_test.hpp>
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_dictionary)

BOOST_AUTO_TEST_CASE(construct)
{
	Dictionary::Ptr dictionary = boost::make_shared<Dictionary>();
	BOOST_CHECK(dictionary);
}

BOOST_AUTO_TEST_CASE(getproperty)
{
	Dictionary::Ptr dictionary = boost::make_shared<Dictionary>();
	dictionary->Set("test1", 7);
	dictionary->Set("test2", "hello world");

	Value test1;
	test1 = dictionary->Get("test1");
	BOOST_CHECK(test1 == 7);

	Value test2;
	test2 = dictionary->Get("test2");
	BOOST_CHECK(test2 == "hello world");

	Value test3;
	test3 = dictionary->Get("test3");
	BOOST_CHECK(test3.IsEmpty());
}

BOOST_AUTO_TEST_CASE(getproperty_dict)
{
	Dictionary::Ptr dictionary = boost::make_shared<Dictionary>();
	Dictionary::Ptr other = boost::make_shared<Dictionary>();

	dictionary->Set("test1", other);

	Dictionary::Ptr test1 = dictionary->Get("test1");
	BOOST_CHECK(other == test1);

	Dictionary::Ptr test2 = dictionary->Get("test2");
	BOOST_CHECK(!test2);
}

BOOST_AUTO_TEST_SUITE_END()
