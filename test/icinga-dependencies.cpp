/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "icinga/host.hpp"
#include "icinga/dependency.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_dependencies)

static Host::Ptr CreateHost(const std::string& name)
{
	Host::Ptr host = new Host();
	host->SetName(name);
	return host;
}

static Dependency::Ptr CreateDependency(Checkable::Ptr parent, Checkable::Ptr child, const std::string& name)
{
	Dependency::Ptr dep = new Dependency();
	dep->SetParent(parent);
	dep->SetChild(child);
	dep->SetName(name + "!" + child->GetName());
	return dep;
}

static void RegisterDependency(Dependency::Ptr dep, const std::string& redundancyGroup)
{
	dep->SetRedundancyGroup(redundancyGroup);
	DependencyGroup::Register(dep);
	dep->GetParent()->AddReverseDependency(dep);
}

static void AssertCheckableRedundancyGroup(Checkable::Ptr checkable, int dependencyCount, int groupCount, int totalDependenciesCount)
{
	BOOST_CHECK_MESSAGE(
		dependencyCount == checkable->GetDependencies().size(),
		"Dependency count mismatch for '" << checkable->GetName() << "' - expected=" << dependencyCount << "; got="
			<< checkable->GetDependencies().size()
	);
	auto dependencyGroups(checkable->GetDependencyGroups());
	BOOST_CHECK_MESSAGE(
		groupCount == dependencyGroups.size(),
		"Dependency group count mismatch for '" << checkable->GetName() << "'" << " - expected=" << groupCount
			<< "; got=" << dependencyGroups.size()
	);
	if (groupCount > 0) {
		BOOST_REQUIRE_MESSAGE(1 <= dependencyGroups.size(), "Checkable '" << checkable->GetName() << "' should have at least one dependency group.");
		BOOST_CHECK_MESSAGE(
			totalDependenciesCount == dependencyGroups.begin()->get()->GetDependenciesCount(),
			"Member count mismatch for '" << checkable->GetName() << "'" << " - expected=" << totalDependenciesCount
				<< "; got=" <<  dependencyGroups.begin()->get()->GetDependenciesCount()
		);
	}
}

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
	Host::Ptr parentHost1 = CreateHost("parentHost1");
	parentHost1->SetStateRaw(ServiceCritical);
	parentHost1->SetStateType(StateTypeHard);
	parentHost1->SetLastCheckResult(new CheckResult());

	Host::Ptr parentHost2 = CreateHost("parentHost2");
	parentHost2->SetStateRaw(ServiceOK);
	parentHost2->SetStateType(StateTypeHard);
	parentHost2->SetLastCheckResult(new CheckResult());

	Host::Ptr childHost = CreateHost("childHost");
	childHost->SetStateRaw(ServiceOK);
	childHost->SetStateType(StateTypeHard);

	/* Build the dependency tree. */
	Dependency::Ptr dep1 (CreateDependency(parentHost1, childHost, "dep1"));
	dep1->SetStateFilter(StateFilterUp);
	RegisterDependency(dep1, "");

	Dependency::Ptr dep2 (CreateDependency(parentHost2, childHost, "dep2"));
	dep2->SetStateFilter(StateFilterUp);
	RegisterDependency(dep2, "");

	/* Test the reachability from this point.
	 * parentHost1 is DOWN, parentHost2 is UP.
	 * Expected result: childHost is unreachable.
	 */
	parentHost1->SetStateRaw(ServiceCritical); // parent Host 1 DOWN
	parentHost2->SetStateRaw(ServiceOK);       // parent Host 2 UP

	BOOST_CHECK(childHost->IsReachable() == false);

	Dependency::Ptr duplicateDep (CreateDependency(parentHost1, childHost, "dep4"));
	duplicateDep->SetIgnoreSoftStates(false, true);
	RegisterDependency(duplicateDep, "");
	parentHost1->SetStateType(StateTypeSoft);

	// It should still be unreachable, due to the duplicated dependency object above with ignore_soft_states set to false.
	BOOST_CHECK(childHost->IsReachable() == false);
	parentHost1->SetStateType(StateTypeHard);
	DependencyGroup::Unregister(duplicateDep);

	/* The only DNS server is DOWN.
	 * Expected result: childHost is unreachable.
	 */
	DependencyGroup::Unregister(dep1); // Remove the dep and re-add it with a configured redundancy group.
	RegisterDependency(dep1, "DNS");
	BOOST_CHECK(childHost->IsReachable() == false);

	/* 1/2 DNS servers is DOWN.
	 * Expected result: childHost is reachable.
	 */
	DependencyGroup::Unregister(dep2);
	RegisterDependency(dep2, "DNS");
	BOOST_CHECK(childHost->IsReachable() == true);

	auto grandParentHost(CreateHost("GrandParentHost"));
	grandParentHost->SetLastCheckResult(new CheckResult());
	grandParentHost->SetStateRaw(ServiceCritical);
	grandParentHost->SetStateType(StateTypeHard);

	Dependency::Ptr dep3 (CreateDependency(grandParentHost, parentHost1, "dep3"));
	dep3->SetStateFilter(StateFilterUp);
	RegisterDependency(dep3, "");
	// The grandparent is DOWN but the DNS redundancy group has to be still reachable.
	BOOST_CHECK_EQUAL(true, childHost->IsReachable());
	DependencyGroup::Unregister(dep3);

	/* Both DNS servers are DOWN.
	 * Expected result: childHost is unreachable.
	 */
	parentHost1->SetStateRaw(ServiceCritical); // parent Host 1 DOWN
	parentHost2->SetStateRaw(ServiceCritical); // parent Host 2 DOWN

	BOOST_CHECK(childHost->IsReachable() == false);
}

BOOST_AUTO_TEST_CASE(default_redundancy_group_registration_unregistration)
{
	Checkable::Ptr childHostC(CreateHost("C"));
	Dependency::Ptr depCA(CreateDependency(CreateHost("A"), childHostC, "depCA"));
	RegisterDependency(depCA, "");
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depCB(CreateDependency(CreateHost("B"), childHostC, "depCB"));
	RegisterDependency(depCB, "");
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 2);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Checkable::Ptr childHostD(CreateHost("D"));
	Dependency::Ptr depDA(CreateDependency(depCA->GetParent(), childHostD, "depDA"));
	RegisterDependency(depDA, "");
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depDB(CreateDependency(depCB->GetParent(), childHostD, "depDB"));
	RegisterDependency(depDB, "");
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depCA);
	DependencyGroup::Unregister(depDA);
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 2);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depCB);
	DependencyGroup::Unregister(depDB);
	AssertCheckableRedundancyGroup(childHostC, 0, 0, 0);
	AssertCheckableRedundancyGroup(childHostD, 0, 0, 0);
	BOOST_CHECK_EQUAL(0, DependencyGroup::GetRegistrySize());
}

BOOST_AUTO_TEST_CASE(simple_redundancy_group_registration_unregistration)
{
	Checkable::Ptr childHostC(CreateHost("childC"));

	Dependency::Ptr depCA(CreateDependency(CreateHost("A"), childHostC, "depCA"));
	RegisterDependency(depCA, "redundant");
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depCB(CreateDependency(CreateHost("B"), childHostC, "depCB"));
	RegisterDependency(depCB, "redundant");
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 2);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Checkable::Ptr childHostD(CreateHost("childD"));
	Dependency::Ptr depDA (CreateDependency(depCA->GetParent(), childHostD, "depDA"));
	RegisterDependency(depDA, "redundant");
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depDB(CreateDependency(depCB->GetParent(), childHostD, "depDB"));
	RegisterDependency(depDB, "redundant");
	// Still 1 redundancy group, but there should be 4 dependencies now, i.e. 2 for each child Checkable.
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depCA);
	// After unregistering depCA, childHostC should have a new redundancy group with only depCB as dependency, and...
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	// ...childHostD should still have the same redundancy group as before but also with only two dependencies.
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 2);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depDA);
	// Nothing should have changed for childHostC, but childHostD should now have a fewer group dependency, i.e.
	// both child hosts should have the same redundancy group with only depCB and depDB as dependency.
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 2);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	DependencyGroup::Register(depDA);
	DependencyGroup::Unregister(depDB);
	// Nothing should have changed for childHostC, but both should now have a separate group with only depCB and depDA as dependency.
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depCB);
	DependencyGroup::Unregister(depDA);
	AssertCheckableRedundancyGroup(childHostC, 0, 0, 0);
	AssertCheckableRedundancyGroup(childHostD, 0, 0, 0);
	BOOST_CHECK_EQUAL(0, DependencyGroup::GetRegistrySize());
}

BOOST_AUTO_TEST_CASE(mixed_redundancy_group_registration_unregsitration)
{
	Checkable::Ptr childHostC(CreateHost("childC"));
	Dependency::Ptr depCA(CreateDependency(CreateHost("A"), childHostC, "depCA"));
	RegisterDependency(depCA, "redundant");
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Checkable::Ptr childHostD(CreateHost("childD"));
	Dependency::Ptr depDA(CreateDependency(depCA->GetParent(), childHostD, "depDA"));
	RegisterDependency(depDA, "redundant");
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depCB(CreateDependency(CreateHost("B"), childHostC, "depCB"));
	RegisterDependency(depCB, "redundant");
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 2);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depDB(CreateDependency(depCB->GetParent(), childHostD, "depDB"));
	RegisterDependency(depDB, "redundant");
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Checkable::Ptr childHostE(CreateHost("childE"));
	Dependency::Ptr depEA(CreateDependency(depCA->GetParent(), childHostE, "depEA"));
	RegisterDependency(depEA, "redundant");
	AssertCheckableRedundancyGroup(childHostE, 1, 1, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depEB(CreateDependency(depCB->GetParent(), childHostE, "depEB"));
	RegisterDependency(depEB, "redundant");
	// All 3 hosts share the same group, and each host has 2 dependencies, thus 6 dependencies in total.
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 6);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 6);
	AssertCheckableRedundancyGroup(childHostE, 2, 1, 6);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depEZ(CreateDependency(CreateHost("Z"), childHostE, "depEZ"));
	RegisterDependency(depEZ, "redundant");
	// Child host E should have a new redundancy group with 3 dependencies and the other two should still share the same group.
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostE, 3, 1, 3);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depEA);
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostE, 2, 1, 2);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	DependencyGroup::Register(depEA); // Re-register depEA and instead...
	DependencyGroup::Unregister(depEZ); // ...unregister depEZ and check if all the hosts share the same group again.
	// All 3 hosts share the same group again, and each host has 2 dependencies, thus 6 dependencies in total.
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 6);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 6);
	AssertCheckableRedundancyGroup(childHostE, 2, 1, 6);
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depCA);
	DependencyGroup::Unregister(depDB);
	DependencyGroup::Unregister(depEB);
	// Child host C has now a separate group with only depCB as dependency, and child hosts D and E share the same group.
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	AssertCheckableRedundancyGroup(childHostE, 1, 1, 2);
	// Child host C has now a separate group with only depCB as member, and child hosts D and E share the same group.
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	DependencyGroup::Unregister(depCB);
	DependencyGroup::Unregister(depDA);
	DependencyGroup::Unregister(depEA);
	AssertCheckableRedundancyGroup(childHostC, 0, 0, 0);
	AssertCheckableRedundancyGroup(childHostD, 0, 0, 0);
	AssertCheckableRedundancyGroup(childHostE, 0, 0, 0);
	BOOST_CHECK_EQUAL(0, DependencyGroup::GetRegistrySize());
}

BOOST_AUTO_TEST_SUITE_END()
