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

#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_array)

BOOST_AUTO_TEST_CASE(construct)
{
	Array::Ptr array = new Array();
	BOOST_CHECK(array);
	BOOST_CHECK(array->GetLength() == 0);
}

BOOST_AUTO_TEST_CASE(getset)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);
	BOOST_CHECK(array->GetLength() == 3);
	BOOST_CHECK(array->Get(0) == 7);
	BOOST_CHECK(array->Get(1) == 2);
	BOOST_CHECK(array->Get(2) == 5);

	array->Set(1, 9);
	BOOST_CHECK(array->Get(1) == 9);

	array->Remove(1);
	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(1) == 5);
}

BOOST_AUTO_TEST_CASE(resize)
{
	Array::Ptr array = new Array();
	array->Resize(2);
	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(0) == Empty);
	BOOST_CHECK(array->Get(1) == Empty);
}

BOOST_AUTO_TEST_CASE(insert)
{
	Array::Ptr array = new Array();

	array->Insert(0, 11);
	array->Insert(1, 22);
	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(1) == 22);

	array->Insert(0, 33);
	BOOST_CHECK(array->GetLength() == 3);
	BOOST_CHECK(array->Get(0) == 33);
	BOOST_CHECK(array->Get(1) == 11);

	array->Insert(1, 44);
	BOOST_CHECK(array->GetLength() == 4);
	BOOST_CHECK(array->Get(0) == 33);
	BOOST_CHECK(array->Get(1) == 44);
	BOOST_CHECK(array->Get(2) == 11);
}

BOOST_AUTO_TEST_CASE(remove)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	{
		ObjectLock olock(array);
		auto it = array->Begin();
		array->Remove(it);
	}

	BOOST_CHECK(array->GetLength() == 2);
	BOOST_CHECK(array->Get(0) == 2);

	array->Clear();
	BOOST_CHECK(array->GetLength() == 0);
}

BOOST_AUTO_TEST_CASE(foreach)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	ObjectLock olock(array);

	int n = 0;

	for (const Value& item : array) {
		BOOST_CHECK(n != 0 || item == 7);
		BOOST_CHECK(n != 1 || item == 2);
		BOOST_CHECK(n != 2 || item == 5);

		n++;
	}
}

BOOST_AUTO_TEST_CASE(clone)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	Array::Ptr clone = array->ShallowClone();

	BOOST_CHECK(clone->GetLength() == 3);
	BOOST_CHECK(clone->Get(0) == 7);
	BOOST_CHECK(clone->Get(1) == 2);
	BOOST_CHECK(clone->Get(2) == 5);
}

BOOST_AUTO_TEST_CASE(json)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(2);
	array->Add(5);

	String json = JsonEncode(array);
	BOOST_CHECK(json.GetLength() > 0);

	Array::Ptr deserialized = JsonDecode(json);
	BOOST_CHECK(deserialized);
	BOOST_CHECK(deserialized->GetLength() == 3);
	BOOST_CHECK(deserialized->Get(0) == 7);
	BOOST_CHECK(deserialized->Get(1) == 2);
	BOOST_CHECK(deserialized->Get(2) == 5);
}

BOOST_AUTO_TEST_SUITE_END()
