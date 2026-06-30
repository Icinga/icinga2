// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/perfdatavalue.hpp"
#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/serializer.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_serialize)

BOOST_AUTO_TEST_CASE(scalar)
{
	BOOST_CHECK(Deserialize(Serialize(7)) == 7);
	BOOST_CHECK(Deserialize(Serialize(7.3)) == 7.3);
	BOOST_CHECK(Deserialize(Serialize(Empty)) == Empty);
	BOOST_CHECK(Deserialize(Serialize("hello")) == "hello");
}

BOOST_AUTO_TEST_CASE(array)
{
	Array::Ptr array = new Array();
	array->Add(7);
	array->Add(7.3);
	array->Add(Empty);
	array->Add("hello");

	Array::Ptr result = Deserialize(Serialize(array));

	BOOST_CHECK(result->GetLength() == array->GetLength());

	BOOST_CHECK(result->Get(0) == 7);
	BOOST_CHECK(result->Get(1) == 7.3);
	BOOST_CHECK(result->Get(2) == Empty);
	BOOST_CHECK(result->Get(3) == "hello");
}

BOOST_AUTO_TEST_CASE(dictionary)
{
	Dictionary::Ptr dict = new Dictionary();
	dict->Set("k1", 7);
	dict->Set("k2", 7.3);
	dict->Set("k3", Empty);
	dict->Set("k4", "hello");

	Dictionary::Ptr result = Deserialize(Serialize(dict));

	BOOST_CHECK(result->GetLength() == dict->GetLength());

	BOOST_CHECK(result->Get("k1") == 7);
	BOOST_CHECK(result->Get("k2") == 7.3);
	BOOST_CHECK(result->Get("k3") == Empty);
	BOOST_CHECK(result->Get("k4") == "hello");
}

BOOST_AUTO_TEST_CASE(object)
{
	PerfdataValue::Ptr pdv = new PerfdataValue("size", 100, true, "bytes");

	PerfdataValue::Ptr result = Deserialize(Serialize(pdv));

	BOOST_CHECK(result->GetValue() == pdv->GetValue());
}

BOOST_AUTO_TEST_SUITE_END()
