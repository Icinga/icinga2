/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/dependency.hpp"
#include "base/logger.hpp"

using namespace icinga;

/**
 * Register all the dependency groups of the current Checkable to the global dependency group registry.
 *
 * Initially, each Checkable object tracks locally its own dependency groups on Icinga 2 startup, and once the start
 * signal of that Checkable is emitted, it pushes all the local tracked dependency groups to the global registry.
 * Once the global registry is populated with all the local dependency groups, this Checkable may not necessarily
 * contain the exact same dependency groups as it did before, as identical groups are merged together in the registry,
 * but it's guaranteed to have the same *number* of dependency groups as before.
 */
void Checkable::PushDependencyGroupsToRegistry()
{
	std::lock_guard lock(m_DependencyMutex);
	if (m_PendingDependencies != nullptr) {
		for (const auto& [key, dependencies] : *m_PendingDependencies) {
			String redundancyGroup = std::holds_alternative<String>(key) ? std::get<String>(key) : "";
			m_DependencyGroups.emplace(key, DependencyGroup::Register(new DependencyGroup(redundancyGroup, dependencies)));
		}
		m_PendingDependencies.reset();
	}
}

std::vector<DependencyGroup::Ptr> Checkable::GetDependencyGroups() const
{
	std::lock_guard lock(m_DependencyMutex);

	std::vector<DependencyGroup::Ptr> dependencyGroups;
	for (const auto& [_, dependencyGroup] : m_DependencyGroups) {
		dependencyGroups.emplace_back(dependencyGroup);
	}
	return dependencyGroups;
}

/**
 * Get the key for the provided dependency group.
 *
 * The key is either the parent Checkable object or the redundancy group name of the dependency object.
 * This is used to uniquely identify the dependency group within a given Checkable object.
 *
 * @param dependency The dependency to get the key for.
 *
 * @return - Returns the key for the provided dependency group.
 */
static std::variant<Checkable*, String> GetDependencyGroupKey(const Dependency::Ptr& dependency)
{
	if (auto redundancyGroup(dependency->GetRedundancyGroup()); !redundancyGroup.IsEmpty()) {
		return std::move(redundancyGroup);
	}

	return dependency->GetParent().get();
}

/**
 * Add the provided dependency to the current Checkable list of dependencies.
 *
 * @param dependency The dependency to add.
 */
void Checkable::AddDependency(const Dependency::Ptr& dependency)
{
	std::unique_lock lock(m_DependencyMutex);

	auto dependencyGroupKey(GetDependencyGroupKey(dependency));
	if (m_PendingDependencies != nullptr) {
		(*m_PendingDependencies)[dependencyGroupKey].emplace(dependency);
		return;
	}

	std::set<Dependency::Ptr> dependencies;
	bool removeGroup(false);

	DependencyGroup::Ptr existingGroup;
	if (auto it(m_DependencyGroups.find(dependencyGroupKey)); it != m_DependencyGroups.end()) {
		existingGroup = it->second;
		std::tie(dependencies, removeGroup) = DependencyGroup::Unregister(existingGroup, this);
		m_DependencyGroups.erase(it);
	}

	dependencies.emplace(dependency);

	auto dependencyGroup(DependencyGroup::Register(new DependencyGroup(dependency->GetRedundancyGroup(), dependencies)));
	m_DependencyGroups.emplace(dependencyGroupKey, dependencyGroup);

	lock.unlock();

	if (existingGroup) {
		dependencies.erase(dependency);
		DependencyGroup::OnChildRemoved(existingGroup, {dependencies.begin(), dependencies.end()}, removeGroup);
	}
	DependencyGroup::OnChildRegistered(this, dependencyGroup);
}

/**
 * Remove the provided dependency from the current Checkable list of dependencies.
 *
 * @param dependency The dependency to remove.
 * @param runtimeRemoved Whether the given dependency object is being removed at runtime.
 */
void Checkable::RemoveDependency(const Dependency::Ptr& dependency, bool runtimeRemoved)
{
	std::unique_lock lock(m_DependencyMutex);

	auto dependencyGroupKey(GetDependencyGroupKey(dependency));
	auto it = m_DependencyGroups.find(dependencyGroupKey);
	if (it == m_DependencyGroups.end()) {
		return;
	}

	DependencyGroup::Ptr existingGroup(it->second);
	auto [dependencies, removeGroup] = DependencyGroup::Unregister(existingGroup, this);

	m_DependencyGroups.erase(it);
	dependencies.erase(dependency);

	DependencyGroup::Ptr newDependencyGroup;
	if (!dependencies.empty()) {
		newDependencyGroup = DependencyGroup::Register(new DependencyGroup(dependency->GetRedundancyGroup(), dependencies));
		m_DependencyGroups.emplace(dependencyGroupKey, newDependencyGroup);
	}

	lock.unlock();

	if (runtimeRemoved) {
		dependencies.emplace(dependency);
		DependencyGroup::OnChildRemoved(existingGroup, {dependencies.begin(), dependencies.end()}, removeGroup);

		if (newDependencyGroup) {
			DependencyGroup::OnChildRegistered(this, newDependencyGroup);
		}
	}
}

std::vector<Dependency::Ptr> Checkable::GetDependencies(bool includePending) const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	std::vector<Dependency::Ptr> dependencies;

	if (includePending && m_PendingDependencies != nullptr) {
		for (const auto& [group, groupDeps] : *m_PendingDependencies) {
			dependencies.insert(dependencies.end(), groupDeps.begin(), groupDeps.end());
		}
	}

	for (const auto& [_, dependencyGroup] : m_DependencyGroups) {
		auto tmpDependencies(dependencyGroup->GetDependenciesForChild(this));
		dependencies.insert(dependencies.end(), tmpDependencies.begin(), tmpDependencies.end());
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

/**
 * Checks whether this checkable is currently reachable according to its dependencies.
 *
 * @param dt Dependency type to evaluate for.
 * @return Whether the given checkable is reachable.
 */
bool Checkable::IsReachable(DependencyType dt) const
{
	return DependencyStateChecker(dt).IsReachable(this);
}

/**
 * Checks whether the last check result of this Checkable affects its child dependencies.
 *
 * A Checkable affects its child dependencies if it runs into a non-OK state and results in any of its child
 * Checkables to become unreachable. Though, that unavailable dependency may not necessarily cause the child
 * Checkable to be in unreachable state as it might have some other dependencies that are still reachable, instead
 * it just indicates whether the edge/connection between this and the child Checkable is broken or not.
 *
 * @return bool - Returns true if the Checkable affects its child dependencies, otherwise false.
 */
bool Checkable::AffectsChildren() const
{
	if (!GetLastCheckResult() || !IsReachable()) {
		// If there is no check result, or the Checkable is not reachable, we can't safely determine whether
		// the Checkable affects its child dependencies.
		return false;
	}

	for (auto& dep : GetReverseDependencies()) {
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
	for (auto& dependencyGroup : GetDependencyGroups()) {
		dependencyGroup->LoadParents(parents);
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
	std::set<Checkable::Ptr> children;

	GetAllChildrenInternal(children, 0);

	return children;
}

/**
 * Retrieve all direct and indirect children of the current Checkable.
 *
 * Note, this function performs a recursive call chain traversing all the children of the current Checkable
 * up to a certain limit (256). When that limit is reached, it will log a warning message and abort the operation.
 *
 * @param seenChildren - A container to store all the traversed children into.
 * @param level - The current level of recursion.
 */
void Checkable::GetAllChildrenInternal(std::set<Checkable::Ptr>& seenChildren, int level) const
{
	if (level > Dependency::MaxDependencyRecursionLevel) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << Dependency::MaxDependencyRecursionLevel << ") for checkable '"
			<< GetName() << "': aborting traversal.";
		return;
	}

	for (const Checkable::Ptr& checkable : GetChildren()) {
		if (auto [_, inserted] = seenChildren.insert(checkable); inserted) {
			checkable->GetAllChildrenInternal(seenChildren, level + 1);
		}
	}
}
