/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/dependency.hpp"
#include "base/logger.hpp"

using namespace icinga;

/**
 * The maximum number of dependency recursion levels allowed.
 *
 * This is a subjective limit how deep the dependency tree should be allowed to go, as anything beyond this level
 * is just madness and will likely result in a stack overflow or other undefined behavior.
 */
static constexpr int l_MaxDependencyRecursionLevel(256);

/**
 * Register all the dependency groups of the current Checkable to the global dependency group registry.
 *
 * Initially, each Checkable object tracks locally its own dependency groups on Icinga 2 startup, and once the start
 * signal of that Checkable is emitted, it pushes all the local tracked dependency groups to the global registry.
 * Once the global registry is populated with all the local dependency groups, this Checkable may not necessarily
 * contain the exact same dependency groups as it did before, as identical groups are merged together in the registry,
 * but it's guaranteed to have the same *number* of dependency groups as before.
 *
 * Note: Re-registering the very same dependency groups to global registry will crash the process with superior error
 * messages that aren't easy to debug, so make sure to never call this method more than once for the same Checkable.
 */
void Checkable::PushDependencyGroupsToRegistry()
{
	std::lock_guard lock(m_DependencyMutex);
	decltype(m_DependencyGroups) dependencyGroups;
	m_DependencyGroups.swap(dependencyGroups);

	for (auto& dependencyGroup : dependencyGroups) {
		m_DependencyGroups.emplace(DependencyGroup::Register(dependencyGroup));
	}
}

std::vector<DependencyGroup::Ptr> Checkable::GetDependencyGroups() const
{
	std::unique_lock lock(m_DependencyMutex);
	return {m_DependencyGroups.begin(), m_DependencyGroups.end()};
}

/**
 * Add the provided dependency to the current Checkable list of dependencies.
 *
 * Returns an existing dependency group that has been modified due to the addition of the new dependency object.
 * Meaning, either the dependency groups identity has changed after adding the new dependency to it, or the provided
 * dependency caused the creation of a new dependency group and decoupling from of the existing one. In the latter case,
 * the returned value is the previously existing group that the current Checkable was member of.
 *
 * @param dependency The dependency to add.
 *
 * @return DependencyGroup::Ptr The dependency group that has been modified.
 */
DependencyGroup::Ptr Checkable::AddDependency(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_DependencyMutex);
	DependencyGroup::Ptr newGroup(new DependencyGroup(dependency->GetRedundancyGroup(), dependency));
	if (auto it(m_DependencyGroups.find(newGroup)); it == m_DependencyGroups.end()) {
		// If the current Checkable is already started (all local dependency groups are already pushed
		// to the global registry), we need to directly forward newGroup to the global register.
		m_DependencyGroups.emplace(IsActive() ? DependencyGroup::Register(newGroup) : newGroup);
	} else if (!IsActive()) {
		// If we're not going to refresh the global registry, we just need to add the dependency to the existing group.
		// Meaning, the dependency group itself isn't registered globally yet, so we don't need to re-register it.
		(*it)->AddMember(dependency);
		return *it;
	} else {
		if (auto existingGroup(*it); existingGroup->HasIdenticalMember(dependency)) {
			// There's already an identical member in the group and this is likely an exact duplicate of it,
			// so it won't change the identity of the group after registration, i.e. regardless whether we're
			// supposed to refresh the global registry or not it's identity will remain the same.
			existingGroup->AddMember(dependency);
		} else {
			// We're going to either replace the existing group with "newGroup" or merge the two groups together.
			// Either way, it's identity will change, so we need to decouple it from the current Checkable.
			m_DependencyGroups.erase(it);

			// We need to unregister the existing group from the global registry if we're the only member of it,
			// as it's hash might change after adding the new dependency to it down below, and we want to re-register
			// it afterwards. This way, we'll also be able to eliminate the possibility of having two identical groups
			// in the registry that might occur due to the registration of the new dependency object below.
			if (DependencyGroup::Unregister(existingGroup, this)) {
				// The current Checkable is the only member of the group, so nothing to move around, just
				// add the _duplicate_ dependency to the existing group. Duplicate in the sense that it's
				// not identical to any of the existing members but similar enough to be part of the same
				// group, i.e. same parent, maybe different period, state filter, etc.
				existingGroup->AddMember(dependency);
				m_DependencyGroups.emplace(DependencyGroup::Register(existingGroup));
			} else {
				// Obviously, the current Checkable is not the only member of the existing group, and it's going to
				// have more members than the other child Checkables in that group after adding the new dependency
				// to it. So, we need to move all the members this Checkable depends on to newGroup and leave the
				// existing group as-is, i.e. it's identity won't change afterwards.
				for (auto& member : existingGroup->GetMembers(this)) {
					existingGroup->RemoveMember(member);
					newGroup->AddMember(member);
				}
				m_DependencyGroups.emplace(DependencyGroup::Register(newGroup));
			}

			// In both of the above cases, the identity of the existing group is going to probably change,
			// so we'll need to clean up all the database entries/relations referencing its old identity.
			return existingGroup;
		}
	}

	return nullptr;
}

/**
 * Remove the provided dependency from the current Checkable list of dependencies.
 *
 * Removing a dependency from the current Checkable almost always involves refreshing the global registry as well,
 * regardless of whether it's a runtime deletion or not. However, it is a bit cheaper/easier operation compared to
 * manually identifying and merging the dependency groups.
 *
 * Returns the dependency group the provided dependency object was member of / is removed from, otherwise nullptr.
 *
 * @param dependency The dependency to remove.
 *
 * @return DependencyGroup::Ptr The dependency group the provided dependency object was removed from.
 */
DependencyGroup::Ptr Checkable::RemoveDependency(const Dependency::Ptr& dependency)
{
	std::lock_guard lock(m_DependencyMutex);
	DependencyGroup::Ptr newGroup(new DependencyGroup(dependency->GetRedundancyGroup(), dependency));
	if (auto it(m_DependencyGroups.find(newGroup)); it != m_DependencyGroups.end()) {
		// Obviously, this method is concerned with removing a dependency from the current Checkable, but we've
		// initiated a new group with the _to be removed_ dependency in it mainly to perform lookups in the set.
		// Now, that we've found the existing group it's member of, we need to remove the dependency from it first.
		newGroup->RemoveMember(dependency);

		auto existingGroup(*it);
		// We're going to either replace or re-register the existing group down below, so we need to decouple it.
		// However, only after we've created another reference to it (see existingGroup above), so that we don't
		// get a dangling pointer to it in case this was the only reference to it.
		m_DependencyGroups.erase(it);

		// This is a very similar case to the one in AddDependency, but we're going to remove the dependency
		// from the group instead of adding it. Therefore, most of the reasoning given there apply here as well.
		if (DependencyGroup::Unregister(existingGroup, this)) {
			existingGroup->RemoveMember(dependency);
			newGroup = existingGroup;
		} else {
			// The current Checkable is not the only member of the existing group, and it's going to have
			// (more or less) fewer members than the other child Checkables within that group after unregistering the
			// dependency from it. Meaning, in case the current Checkable have more than one identical dependency in
			// the group, it'll end up having the very same dependency group as before, but with one less member.
			// However, instead of determining such edge cases manually, we're taking the easy way out, i.e. just move
			// all the members (except the one to be removed) to newGroup and re-register it in the global registry.
			for (auto& member : existingGroup->GetMembers(this)) {
				existingGroup->RemoveMember(member);
				if (member != dependency) {
					newGroup->AddMember(member);
				}
			}
		}

		if (newGroup->HasMembers()) {
			m_DependencyGroups.emplace(DependencyGroup::Register(newGroup));
		}
		return existingGroup;
	}

	return nullptr;
}

std::vector<Dependency::Ptr> Checkable::GetDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	std::vector<Dependency::Ptr> dependencies;
	for (const auto& dependencyGroup : m_DependencyGroups) {
		auto members(dependencyGroup->GetMembers(this));
		dependencies.insert(dependencies.end(), members.begin(), members.end());
	}

	return dependencies;
}

bool Checkable::HasAnyDependencies() const
{
	std::unique_lock lock(m_DependencyMutex);
	return !m_DependencyGroups.empty() || !m_ReverseDependencies.empty();
}

void Checkable::AddReverseDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	m_ReverseDependencies.insert(dep);
}

void Checkable::RemoveReverseDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	m_ReverseDependencies.erase(dep);
}

std::vector<Dependency::Ptr> Checkable::GetReverseDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	return std::vector<Dependency::Ptr>(m_ReverseDependencies.begin(), m_ReverseDependencies.end());
}

bool Checkable::IsReachable(DependencyType dt, int rstack) const
{
	if (rstack > l_MaxDependencyRecursionLevel) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << l_MaxDependencyRecursionLevel << ") for checkable '" << GetName() << "': Dependency failed.";

		return false;
	}

	/* implicit dependency on host if this is a service */
	const auto *service = dynamic_cast<const Service *>(this);
	if (service && (dt == DependencyState || dt == DependencyNotification)) {
		Host::Ptr host = service->GetHost();

		if (host && host->GetState() != HostUp && host->GetStateType() == StateTypeHard) {
			return false;
		}
	}

	for (auto& dependencyGroup : GetDependencyGroups()) {
		if (!(dependencyGroup->GetState(dt, rstack + 1) & DependencyGroup::State::ReachableOK)) {
			if (dependencyGroup->IsRedundancyGroup()) { // For non-redundant groups, this should already be logged.
				Log(LogDebug, "Checkable")
					<< "All dependencies in redundancy group '" << dependencyGroup->GetName() << "' have failed for checkable '"
					<< GetName() << "': Marking as unreachable.";
			}
			return false;
		}
	}

	return true;
}

/**
 * Checks whether the last check result of this Checkable affects its child dependencies.
 *
 * A Checkable affects its child dependencies if it runs into a non-OK state and results in any of its child
 * Checkables to become unreachable. Though, that unavailable dependency may not necessarily cause the child
 * Checkable to be in unreachable state as it might have some other depndencies that are still reachable, instead
 * it just indicates whether the edge/connection between this and the child Checkable is broken or not.
 *
 * @return bool - Returns true if the Checkable affects its child dependencies, otherwise false.
 */
bool Checkable::AffectsChildren() const
{
	auto cr(GetLastCheckResult());
	if (!cr || IsStateOK(cr->GetState()) || !IsReachable()) {
		// If there is no check result, the state is OK, or the Checkable is not reachable, we can't
		// safely determine whether the Checkable affects its child dependencies.
		return false;
	}

	for (auto& dep: GetReverseDependencies()) {
		if (!dep->IsAvailable(DependencyState)) {
			// If one of the child dependency is not available, then it's definitely due to the
			// current Checkable state, so we don't need to verify the remaining ones.
			return true;
		}
	}

	return false;
}

std::set<Checkable::Ptr> Checkable::GetParents() const
{
	std::set<Checkable::Ptr> parents;

	for (const Dependency::Ptr& dep : GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (parent && parent.get() != this)
			parents.insert(parent);
	}

	return parents;
}

std::set<Checkable::Ptr> Checkable::GetChildren() const
{
	std::set<Checkable::Ptr> parents;

	for (const Dependency::Ptr& dep : GetReverseDependencies()) {
		Checkable::Ptr service = dep->GetChild();

		if (service && service.get() != this)
			parents.insert(service);
	}

	return parents;
}

/**
 * Retrieve the total number of all the children of the current Checkable.
 *
 * Note, due to the max recursion limit of 256, the returned number may not reflect
 * the actual total number of children involved in the dependency chain.
 *
 * @return int - Returns the total number of all the children of the current Checkable.
 */
size_t Checkable::GetAllChildrenCount() const
{
	// Are you thinking in making this more efficient? Please, don't.
	// In order not to count the same child multiple times, we need to maintain a separate set of visited children,
	// which is basically the same as what GetAllChildren() does. So, we're using it here!
	return GetAllChildren().size();
}

std::set<Checkable::Ptr> Checkable::GetAllChildren() const
{
	std::set<Checkable::Ptr> children = GetChildren();

	GetAllChildrenInternal(children, 0);

	return children;
}

/**
 * Retrieve all direct and indirect children of the current Checkable.
 *
 * Note, this function performs a recursive call chain traversing all the children of the current Checkable
 * up to a certain limit (256). When that limit is reached, it will log a warning message and abort the operation.
 *
 * @param children - The set of children to be filled with all the children of the current Checkable.
 * @param level - The current level of recursion.
 */
void Checkable::GetAllChildrenInternal(std::set<Checkable::Ptr>& children, int level) const
{
	if (level > l_MaxDependencyRecursionLevel) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << l_MaxDependencyRecursionLevel << ") for checkable '" << GetName() << "': aborting traversal.";
		return ;
	}

	std::set<Checkable::Ptr> localChildren;

	for (const Checkable::Ptr& checkable : children) {
		if (auto cChildren(checkable->GetChildren()); !cChildren.empty()) {
			GetAllChildrenInternal(cChildren, level + 1);
			localChildren.insert(cChildren.begin(), cChildren.end());
		}

		if (level != 0) { // Recursion level 0 is the initiator, so checkable is already in the set.
			localChildren.insert(checkable);
		}
	}

	children.insert(localChildren.begin(), localChildren.end());
}

/**
 * Generate the hash of the provided dependency group.
 *
 * Note, the resulted hash is used to uniquely identify the dependency group on a per Checkable basis and not globally.
 * Therefore, for redundancy groups, the hash is generated based on the group name, for non-redundant groups, on the
 * parent Checkable name of its members.
 *
 * @param dependencyGroup The dependency group to generate the hash for.
 *
 * @return size_t - Returns the hash of the provided dependency group.
 */
size_t Checkable::HashDependencyGroup::operator()(const DependencyGroup::Ptr& dependencyGroup) const
{
	return std::hash<String>()(
		dependencyGroup->IsRedundancyGroup() ? dependencyGroup->GetName() : dependencyGroup->PeekParentCheckableName()
	);
}

/**
 * Check the equality of the provided dependency groups.
 *
 * Please note that the equality check is based on criteria that uniquely identify the dependency group
 * within a given Checkable only and aren't meant to be used globally.
 *
 * @param lhs The left-hand side dependency group to compare.
 * @param rhs The right-hand side dependency group to compare.
 *
 * @return bool - Returns true if the provided dependency groups turned to be equal, otherwise false.
 */
bool Checkable::EqualDependencyGroups::operator()(const DependencyGroup::Ptr& lhs, const DependencyGroup::Ptr& rhs) const
{
	if (lhs->IsRedundancyGroup() != rhs->IsRedundancyGroup()) {
		return false;
	}

	if (lhs->IsRedundancyGroup()) {
		return lhs->GetName() == rhs->GetName();
	}

	return lhs->PeekParentCheckableName() == rhs->PeekParentCheckableName();
}
