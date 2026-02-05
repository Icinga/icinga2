// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/host.hpp"
#include "icinga/dependency.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_dependencies)

static Host::Ptr CreateHost(const std::string& name, bool pushDependencyGroupsToRegistry = true)
{
	Host::Ptr host = new Host();
	host->SetName(name);
	if (pushDependencyGroupsToRegistry) {
		host->PushDependencyGroupsToRegistry();
	}
	return host;
}

static Dependency::Ptr CreateDependency(Checkable::Ptr parent, Checkable::Ptr child, const String& name)
{
	Dependency::Ptr dep = new Dependency();
	dep->SetParent(parent);
	dep->SetChild(child);
	dep->SetName(name + "!" + child->GetName());
	return dep;
}

static void RegisterDependency(Dependency::Ptr dep, const String& redundancyGroup)
{
	dep->SetRedundancyGroup(redundancyGroup);
	dep->GetChild()->AddDependency(dep);
	dep->GetParent()->AddReverseDependency(dep);
}

static void AssertCheckableRedundancyGroup(
	Checkable::Ptr checkable,
	std::size_t dependencyCount,
	std::size_t groupCount,
	std::size_t totalDependenciesCount
)
{
	BOOST_CHECK_MESSAGE(
		dependencyCount == checkable->GetDependencies().size(),
		"Dependency count mismatch for '" << checkable->GetName() << "' - expected=" << dependencyCount << "; got="
			<< checkable->GetDependencies().size()
	);
	auto dependencyGroups(checkable->GetDependencyGroups());
	BOOST_CHECK_MESSAGE(
		groupCount == dependencyGroups.size(),
		"Dependency group count mismatch for '" << checkable->GetName() << "' - expected=" << groupCount
			<< "; got=" << dependencyGroups.size()
	);

	for (auto& dependencyGroup : dependencyGroups) {
		BOOST_CHECK_MESSAGE(
			totalDependenciesCount == dependencyGroup->GetDependenciesCount(),
			"Dependency group '" << dependencyGroup->GetRedundancyGroupName() << "' and Checkable '" << checkable->GetName()
				<< "' total dependencies count mismatch - expected=" << totalDependenciesCount << "; got="
				<< dependencyGroup->GetDependenciesCount()
		);
	}

	if (groupCount > 0) {
		BOOST_REQUIRE_MESSAGE(!dependencyGroups.empty(), "Checkable '" << checkable->GetName() << "' should have at least one dependency group.");
	}
}

static std::vector<DependencyGroup::Ptr> ExtractGroups(const Checkable::Ptr& checkable)
{
	auto dependencyGroups(checkable->GetDependencyGroups());
	std::sort(dependencyGroups.begin(), dependencyGroups.end());
	return dependencyGroups;
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
	childHost->RemoveDependency(duplicateDep);

	/* The only DNS server is DOWN.
	 * Expected result: childHost is unreachable.
	 */
	childHost->RemoveDependency(dep1); // Remove the dep and re-add it with a configured redundancy group.
	RegisterDependency(dep1, "DNS");
	BOOST_CHECK(childHost->IsReachable() == false);

	/* 1/2 DNS servers is DOWN.
	 * Expected result: childHost is reachable.
	 */
	childHost->RemoveDependency(dep2);
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
	childHost->RemoveDependency(dep3);

	/* Both DNS servers are DOWN.
	 * Expected result: childHost is unreachable.
	 */
	parentHost1->SetStateRaw(ServiceCritical); // parent Host 1 DOWN
	parentHost2->SetStateRaw(ServiceCritical); // parent Host 2 DOWN

	BOOST_CHECK(childHost->IsReachable() == false);
}

BOOST_AUTO_TEST_CASE(push_dependency_groups_to_registry)
{
	Checkable::Ptr childHostC(CreateHost("C", false));
	Checkable::Ptr childHostD(CreateHost("D", false));
	std::set<Dependency::Ptr> dependencies; // Keep track of all dependencies to avoid unexpected deletions.
	for (auto& parent : {String("A"), String("B"), String("E")}) {
		Dependency::Ptr depC(CreateDependency(CreateHost(parent), childHostC, "depC" + parent));
		Dependency::Ptr depD(CreateDependency(depC->GetParent(), childHostD, "depD" + parent));
		if (parent == "A") {
			Dependency::Ptr depCA2(CreateDependency(depC->GetParent(), childHostC, "depCA2"));
			childHostC->AddDependency(depCA2);
			dependencies.emplace(depCA2);
		} else {
			depC->SetRedundancyGroup("redundant", true);
			depD->SetRedundancyGroup("redundant", true);

			if (parent == "B") { // Create an exact duplicate of depC, but with a different name.
				Dependency::Ptr depCB2(CreateDependency(depC->GetParent(), childHostC, "depCB2"));
				depCB2->SetRedundancyGroup("redundant", true);
				childHostC->AddDependency(depCB2);
				dependencies.emplace(depCB2);
			}
		}
		childHostC->AddDependency(depC);
		childHostD->AddDependency(depD);
		dependencies.insert({depC, depD});
	}

	childHostC->PushDependencyGroupsToRegistry();
	childHostD->PushDependencyGroupsToRegistry();

	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());
	for (auto& checkable : {childHostC, childHostD}) {
		BOOST_CHECK_EQUAL(2, checkable->GetDependencyGroups().size());
		for (auto& dependencyGroup : checkable->GetDependencyGroups()) {
			if (dependencyGroup->IsRedundancyGroup()) {
				BOOST_CHECK_EQUAL(5, dependencyGroup->GetDependenciesCount());
				BOOST_CHECK_EQUAL(checkable == childHostC ? 5 : 3, checkable->GetDependencies().size());
			} else {
				BOOST_CHECK_EQUAL(3, dependencyGroup->GetDependenciesCount());
				BOOST_CHECK_EQUAL(checkable == childHostC ? 5 : 3, checkable->GetDependencies().size());
			}
		}
	}
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
	AssertCheckableRedundancyGroup(childHostC, 2, 2, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	Checkable::Ptr childHostD(CreateHost("D"));
	Dependency::Ptr depDA(CreateDependency(depCA->GetParent(), childHostD, "depDA"));
	RegisterDependency(depDA, "");
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depDB(CreateDependency(depCB->GetParent(), childHostD, "depDB"));
	RegisterDependency(depDB, "");
	AssertCheckableRedundancyGroup(childHostD, 2, 2, 2);
	AssertCheckableRedundancyGroup(childHostC, 2, 2, 2);
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	// This is an exact duplicate of depCA, but with a different dependency name.
	Dependency::Ptr depCA2(CreateDependency(depCA->GetParent(), childHostC, "depCA2"));
	// This is a duplicate of depCA, but with a different state filter.
	Dependency::Ptr depCA3(CreateDependency(depCA->GetParent(), childHostC, "depCA3"));
	depCA3->SetStateFilter(StateFilterUp, true);
	// This is a duplicate of depCA, but with a different ignore_soft_states flag.
	Dependency::Ptr depCA4(CreateDependency(depCA->GetParent(), childHostC, "depCA4"));
	depCA4->SetIgnoreSoftStates(false, true);

	for (auto& dependency : {depCA2, depCA3, depCA4}) {
		bool isAnExactDuplicate = dependency == depCA2;
		RegisterDependency(dependency, "");

		if (isAnExactDuplicate) {
			BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
		}

		for (auto& dependencyGroup : childHostD->GetDependencyGroups()) {
			if (dependency->GetParent() == dependencyGroup->GetDependenciesForChild(childHostD.get()).front()->GetParent()) {
				BOOST_CHECK_EQUAL(isAnExactDuplicate ? 3 : 1, dependencyGroup->GetDependenciesCount());
			} else {
				BOOST_CHECK_EQUAL(2, dependencyGroup->GetDependenciesCount());
			}
			BOOST_CHECK_EQUAL(2, childHostD->GetDependencies().size());
		}

		for (auto& dependencyGroup : childHostC->GetDependencyGroups()) {
			if (dependency->GetParent() == dependencyGroup->GetDependenciesForChild(childHostC.get()).front()->GetParent()) {
				// If depCA2 is currently being processed, then the group should have 3 dependencies, that's because
				// depCA2 is an exact duplicate of depCA, and depCA shares the same group with depDA.
				BOOST_CHECK_EQUAL(isAnExactDuplicate ? 3 : 2, dependencyGroup->GetDependenciesCount());
			} else {
				BOOST_CHECK_EQUAL(2, dependencyGroup->GetDependenciesCount());
			}
			// The 3 dependencies are depCA, depCB, and the current one from the loop.
			BOOST_CHECK_EQUAL(3, childHostC->GetDependencies().size());
		}
		BOOST_CHECK_EQUAL(isAnExactDuplicate ? 2 : 3, DependencyGroup::GetRegistrySize());
		childHostC->RemoveDependency(dependency);
	}

	childHostC->RemoveDependency(depCA);
	childHostD->RemoveDependency(depDA);
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 2);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	childHostC->RemoveDependency(depCB);
	childHostD->RemoveDependency(depDB);
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
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	childHostC->RemoveDependency(depCA);
	// After unregistering depCA, childHostC should have a new redundancy group with only depCB as dependency, and...
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	// ...childHostD should still have the same redundancy group as before but also with only two dependencies.
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 2);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	childHostD->RemoveDependency(depDA);
	// Nothing should have changed for childHostC, but childHostD should now have a fewer group dependency, i.e.
	// both child hosts should have the same redundancy group with only depCB and depDB as dependency.
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 2);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	RegisterDependency(depDA, depDA->GetRedundancyGroup());
	childHostD->RemoveDependency(depDB);
	// Nothing should have changed for childHostC, but both should now have a separate group with only depCB and depDA as dependency.
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 1);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	childHostC->RemoveDependency(depCB);
	childHostD->RemoveDependency(depDA);
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
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
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
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
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
	auto childHostCGroups(ExtractGroups(childHostC));
	BOOST_TEST((childHostCGroups == ExtractGroups(childHostD) && childHostCGroups == ExtractGroups(childHostE)));
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	Dependency::Ptr depEZ(CreateDependency(CreateHost("Z"), childHostE, "depEZ"));
	RegisterDependency(depEZ, "redundant");
	// Child host E should have a new redundancy group with 3 dependencies and the other two should still share the same group.
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	AssertCheckableRedundancyGroup(childHostE, 3, 1, 3);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	childHostE->RemoveDependency(depEA);
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 4);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 4);
	BOOST_TEST(ExtractGroups(childHostC) == ExtractGroups(childHostD), boost::test_tools::per_element());
	AssertCheckableRedundancyGroup(childHostE, 2, 1, 2);
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	RegisterDependency(depEA, depEA->GetRedundancyGroup()); // Re-register depEA and instead...
	childHostE->RemoveDependency(depEZ); // ...unregister depEZ and check if all the hosts share the same group again.
	// All 3 hosts share the same group again, and each host has 2 dependencies, thus 6 dependencies in total.
	AssertCheckableRedundancyGroup(childHostC, 2, 1, 6);
	AssertCheckableRedundancyGroup(childHostD, 2, 1, 6);
	AssertCheckableRedundancyGroup(childHostE, 2, 1, 6);
	childHostCGroups = ExtractGroups(childHostC);
	BOOST_TEST((childHostCGroups == ExtractGroups(childHostD) && childHostCGroups == ExtractGroups(childHostE)));
	BOOST_CHECK_EQUAL(1, DependencyGroup::GetRegistrySize());

	childHostC->RemoveDependency(depCA);
	childHostD->RemoveDependency(depDB);
	childHostE->RemoveDependency(depEB);
	// Child host C has now a separate group with only depCB as dependency, and child hosts D and E share the same group.
	AssertCheckableRedundancyGroup(childHostC, 1, 1, 1);
	AssertCheckableRedundancyGroup(childHostD, 1, 1, 2);
	AssertCheckableRedundancyGroup(childHostE, 1, 1, 2);
	BOOST_TEST(ExtractGroups(childHostD) == ExtractGroups(childHostE), boost::test_tools::per_element());
	BOOST_CHECK_EQUAL(2, DependencyGroup::GetRegistrySize());

	childHostC->RemoveDependency(depCB);
	childHostD->RemoveDependency(depDA);
	childHostE->RemoveDependency(depEA);
	AssertCheckableRedundancyGroup(childHostC, 0, 0, 0);
	AssertCheckableRedundancyGroup(childHostD, 0, 0, 0);
	AssertCheckableRedundancyGroup(childHostE, 0, 0, 0);
	BOOST_CHECK_EQUAL(0, DependencyGroup::GetRegistrySize());
}

BOOST_AUTO_TEST_SUITE_END()
