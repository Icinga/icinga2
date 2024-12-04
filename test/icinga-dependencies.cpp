/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "icinga/host.hpp"
#include "icinga/dependency.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_dependencies)

BOOST_AUTO_TEST_CASE(multi_parent)
{
	/* One child host, two parent hosts. Simulate multi-parent dependencies. */
	std::cout << "Testing reachability for multi parent dependencies." << std::endl;

	/*
	 * Our mock requires:
	 * - SetParent/SetChild functions for the dependency
	 * - Parent objects need a CheckResult object
	 * - Dependencies need a StateFilter
	 */
	Host::Ptr parentHost1 = new Host();
	parentHost1->SetActive(true);
	parentHost1->SetMaxCheckAttempts(1);
	parentHost1->Activate();
	parentHost1->SetAuthority(true);
	parentHost1->SetStateRaw(ServiceCritical);
	parentHost1->SetStateType(StateTypeHard);
	parentHost1->SetLastCheckResult(new CheckResult());

	Host::Ptr parentHost2 = new Host();
	parentHost2->SetActive(true);
	parentHost2->SetMaxCheckAttempts(1);
	parentHost2->Activate();
	parentHost2->SetAuthority(true);
	parentHost2->SetStateRaw(ServiceOK);
	parentHost2->SetStateType(StateTypeHard);
	parentHost2->SetLastCheckResult(new CheckResult());

	Host::Ptr childHost = new Host();
	childHost->SetActive(true);
	childHost->SetMaxCheckAttempts(1);
	childHost->Activate();
	childHost->SetAuthority(true);
	childHost->SetStateRaw(ServiceOK);
	childHost->SetStateType(StateTypeHard);

	/* Build the dependency tree. */
	Dependency::Ptr dep1 = new Dependency();

	dep1->SetParent(parentHost1);
	dep1->SetChild(childHost);
	dep1->SetStateFilter(StateFilterUp);

	// Reverse dependencies
        DependencyGroup::Register(dep1);
        parentHost1->AddReverseDependency(dep1);

	Dependency::Ptr dep2 = new Dependency();

	dep2->SetParent(parentHost2);
	dep2->SetChild(childHost);
	dep2->SetStateFilter(StateFilterUp);

	// Reverse dependencies
        DependencyGroup::Register(dep2);
        parentHost2->AddReverseDependency(dep2);


	/* Test the reachability from this point.
	 * parentHost1 is DOWN, parentHost2 is UP.
	 * Expected result: childHost is unreachable.
	 */
	parentHost1->SetStateRaw(ServiceCritical); // parent Host 1 DOWN
	parentHost2->SetStateRaw(ServiceOK);       // parent Host 2 UP

	BOOST_CHECK(childHost->IsReachable() == false);

	/* The only DNS server is DOWN.
	 * Expected result: childHost is unreachable.
	 */
	dep1->SetRedundancyGroup("DNS");
	BOOST_CHECK(childHost->IsReachable() == false);

	/* 1/2 DNS servers is DOWN.
	 * Expected result: childHost is reachable.
	 */
	dep2->SetRedundancyGroup("DNS");
	BOOST_CHECK(childHost->IsReachable() == true);

	/* Both DNS servers are DOWN.
	 * Expected result: childHost is unreachable.
	 */
	parentHost1->SetStateRaw(ServiceCritical); // parent Host 1 DOWN
	parentHost2->SetStateRaw(ServiceCritical); // parent Host 2 DOWN

	BOOST_CHECK(childHost->IsReachable() == false);
}

BOOST_AUTO_TEST_SUITE_END()
