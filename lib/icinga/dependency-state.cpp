/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "icinga/dependency.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"

using namespace icinga;

/**
 * Construct a helper for evaluating the state of dependencies.
 *
 * @param dt Dependency type to check for within the individual methods.
 */
DependencyStateChecker::DependencyStateChecker(DependencyType dt)
	: m_DependencyType(dt)
{
}

/**
 * Checks whether a given checkable is currently reachable.
 *
 * @param checkable The checkable to check reachability for.
 * @param rstack The recursion stack level to prevent infinite recursion, defaults to 0.
 * @return Whether the given checkable is reachable.
 */
bool DependencyStateChecker::IsReachable(Checkable::ConstPtr checkable, int rstack)
{
	if (rstack > Dependency::MaxDependencyRecursionLevel) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << Dependency::MaxDependencyRecursionLevel << ") for checkable '"
			<< checkable->GetName() << "': Dependency failed.";

		return false;
	}

	/* implicit dependency on host if this is a service */
	const auto* service = dynamic_cast<const Service*>(checkable.get());
	if (service && (m_DependencyType == DependencyState || m_DependencyType == DependencyNotification)) {
		Host::Ptr host = service->GetHost();

		if (host && host->GetState() != HostUp && host->GetStateType() == StateTypeHard) {
			return false;
		}
	}

	for (auto& dependencyGroup : checkable->GetDependencyGroups()) {
		if (auto state(GetState(dependencyGroup, checkable.get(), rstack + 1)); state != DependencyGroup::State::Ok) {
			Log(LogDebug, "Checkable")
				<< "Dependency group '" << dependencyGroup->GetRedundancyGroupName() << "' have failed for checkable '"
				<< checkable->GetName() << "': Marking as unreachable.";

			return false;
		}
	}

	return true;
}

/**
 * Retrieve the state of the given dependency group.
 *
 * The state of the dependency group is determined based on the state of the parent Checkables and dependency objects
 * of the group. A dependency group is considered unreachable when none of the parent Checkables is reachable. However,
 * a dependency group may still be marked as failed even when it has reachable parent Checkables, but an unreachable
 * group has always a failed state.
 *
 * @param group
 * @param child The child Checkable to evaluate the state for.
 * @param rstack The recursion stack level to prevent infinite recursion, defaults to 0.
 *
 * @return - Returns the state of the current dependency group.
 */
DependencyGroup::State DependencyStateChecker::GetState(const DependencyGroup::ConstPtr& group, const Checkable* child, int rstack)
{
	using State = DependencyGroup::State;

	auto dependencies(group->GetDependenciesForChild(child));
	size_t reachable = 0, available = 0;

	for (const auto& dependency : dependencies) {
		if (IsReachable(dependency->GetParent(), rstack)) {
			reachable++;

			// Only reachable parents are considered for availability. If they are unreachable and checks are
			// disabled, they could be incorrectly treated as available otherwise.
			if (dependency->IsAvailable(m_DependencyType)) {
				available++;
			}
		}
	}

	if (group->IsRedundancyGroup()) {
		// The state of a redundancy group is determined by the best state of any parent. If any parent ist reachable,
		// the redundancy group is reachable, analogously for availability.
		if (reachable == 0) {
			return State::Unreachable;
		} else if (available == 0) {
			return State::Failed;
		} else {
			return State::Ok;
		}
	} else {
		// For dependencies without a redundancy group, dependencies.size() will be 1 in almost all cases. It will only
		// contain more elements if there are duplicate dependency config objects between two checkables. In this case,
		// all of them have to be reachable/available as they don't provide redundancy.
		if (reachable < dependencies.size()) {
			return State::Unreachable;
		} else if (available < dependencies.size()) {
			return State::Failed;
		} else {
			return State::Ok;
		}
	}
}
