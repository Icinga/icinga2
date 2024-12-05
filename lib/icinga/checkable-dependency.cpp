/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/dependency.hpp"
#include "base/logger.hpp"
#include <unordered_map>

using namespace icinga;

/**
 * The maximum number of allowed dependency recursion levels.
 *
 * This is a subjective limit how deep the dependency tree should be allowed to go, as anything beyond this level
 * is just madness and will likely result in a stack overflow or other undefined behavior.
 */
static constexpr int l_MaxDependencyRecursionLevel(256);

void Checkable::AddDependencyGroup(const DependencyGroup::Ptr& dependencyGroup)
{
	std::unique_lock lock(m_DependencyMutex);
	m_DependencyGroups.insert(dependencyGroup);
}

void Checkable::RemoveDependencyGroup(const DependencyGroup::Ptr& dependencyGroup)
{
	std::unique_lock lock(m_DependencyMutex);
	m_DependencyGroups.erase(dependencyGroup);
}

void Checkable::AddDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	m_Dependencies.insert(dep);
}

void Checkable::RemoveDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	m_Dependencies.erase(dep);
}

std::vector<Dependency::Ptr> Checkable::GetDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	return std::vector<Dependency::Ptr>(m_Dependencies.begin(), m_Dependencies.end());
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

	for (const Checkable::Ptr& checkable : GetParents()) {
		if (!checkable->IsReachable(dt, rstack + 1))
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

	auto deps = GetDependencies();

	std::unordered_map<std::string, Dependency::Ptr> violated; // key: redundancy group, value: nullptr if satisfied, violating dependency otherwise

	for (const Dependency::Ptr& dep : deps) {
		std::string redundancy_group = dep->GetRedundancyGroup();

		if (!dep->IsAvailable(dt)) {
			if (redundancy_group.empty()) {
				Log(LogDebug, "Checkable")
					<< "Non-redundant dependency '" << dep->GetName() << "' failed for checkable '" << GetName() << "': Marking as unreachable.";

				return false;
			}

			// tentatively mark this dependency group as failed unless it is already marked;
			//  so it either passed before (don't overwrite) or already failed (so don't care)
			// note that std::unordered_map::insert() will not overwrite an existing entry
			violated.insert(std::make_pair(redundancy_group, dep));
		} else if (!redundancy_group.empty()) {
			violated[redundancy_group] = nullptr;
		}
	}

	auto violator = std::find_if(violated.begin(), violated.end(), [](auto& v) { return v.second != nullptr; });
	if (violator != violated.end()) {
		Log(LogDebug, "Checkable")
			<< "All dependencies in redundancy group '" << violator->first << "' have failed for checkable '" << GetName() << "': Marking as unreachable.";

		return false;
	}

	return true;
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
	if (level > l_MaxDependencyRecursionLevel) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << l_MaxDependencyRecursionLevel << ") for checkable '" << GetName() << "': aborting traversal.";
		return;
	}

	for (const Checkable::Ptr& checkable : GetChildren()) {
		if (auto [_, inserted] = seenChildren.insert(checkable); inserted) {
			checkable->GetAllChildrenInternal(seenChildren, level + 1);
		}
	}
}
