// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/perfdatavalue.hpp"
#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/application.hpp"
#include "base/type.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(types)

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

BOOST_AUTO_TEST_CASE(sort_by_load_after)
{
	int totalDependencies{0};
	std::unordered_set<Type*> previousTypes;
	for (auto type : Type::GetConfigTypesSortedByLoadDependencies()) {
		BOOST_CHECK_EQUAL(true, ConfigObject::TypeInstance->IsAssignableFrom(type));

		totalDependencies += type->GetLoadDependencies().size();
		for (Type* dependency : type->GetLoadDependencies()) {
			// Note, Type::GetConfigTypesSortedByLoadDependencies() does not detect any cyclic load dependencies,
			// instead, it relies on this test case to fail.
			BOOST_CHECK_MESSAGE(previousTypes.find(dependency) != previousTypes.end(), "type '" << type->GetName()
				<< "' depends on '"<< dependency->GetName() << "' type, but it's not loaded before");
		}

		previousTypes.emplace(type.get());
	}

	// The magic number 12 is the sum of host,service,comment,downtime and endpoint load_after dependencies.
	BOOST_CHECK_MESSAGE(totalDependencies >= 12, "total size of load dependency must be at least 12");
	BOOST_CHECK_MESSAGE(previousTypes.find(Host::TypeInstance.get()) != previousTypes.end(), "Host type should be in the list");
	BOOST_CHECK_MESSAGE(previousTypes.find(Service::TypeInstance.get()) != previousTypes.end(), "Service type should be in the list");
	BOOST_CHECK_MESSAGE(previousTypes.find(Downtime::TypeInstance.get()) != previousTypes.end(), "Downtime type should be in the list");
	BOOST_CHECK_MESSAGE(previousTypes.find(Comment::TypeInstance.get()) != previousTypes.end(), "Comment type should be in the list");
	BOOST_CHECK_MESSAGE(previousTypes.find(Notification::TypeInstance.get()) != previousTypes.end(), "Notification type should be in the list");
	BOOST_CHECK_MESSAGE(previousTypes.find(Zone::TypeInstance.get()) != previousTypes.end(), "Zone type should be in the list");
	BOOST_CHECK_MESSAGE(previousTypes.find(Endpoint::TypeInstance.get()) != previousTypes.end(), "Endpoint type should be in the list");
}

BOOST_AUTO_TEST_SUITE_END()
