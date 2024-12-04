/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/dependency.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <unordered_map>

using namespace icinga;

// Is the default (dummy) redundancy group used to group non-redundant dependencies.
static String l_DefaultRedundancyGroup(Utility::NewUniqueID());

/**
 * Check if the provided redundancy group matches the randomly generated (default) one.
 *
 * @param group The redundancy group name you want to check.
 *
 * @return bool - Returns true if the provided redundancy group matches the randomly generated (default) one.
 */
bool Checkable::IsDefaultRedundancyGroup(const String& group)
{
	return group == l_DefaultRedundancyGroup;
}

void Checkable::AddDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	auto group(dep->GetRedundancyGroup().IsEmpty() ? l_DefaultRedundancyGroup : dep->GetRedundancyGroup());
	if (m_Dependencies.find(group) == m_Dependencies.end()) {
		m_Dependencies.emplace(std::move(group), std::set({dep}));
	} else {
		m_Dependencies[group].emplace(dep);
	}
}

void Checkable::RemoveDependency(const Dependency::Ptr& dep)
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	auto group(dep->GetRedundancyGroup().IsEmpty() ? l_DefaultRedundancyGroup : dep->GetRedundancyGroup());
	if (auto it(m_Dependencies.find(group)); it != m_Dependencies.end()) {
		it->second.erase(dep);

		if (it->second.empty()) {
			m_Dependencies.erase(group);
		}
	}
}

std::vector<Dependency::Ptr> Checkable::GetDependencies() const
{
	std::unique_lock<std::mutex> lock(m_DependencyMutex);
	std::vector<Dependency::Ptr> dependencies;
	for (const auto& [_, deps] : m_Dependencies) {
		std::copy(deps.begin(), deps.end(), std::back_inserter(dependencies));
	}

	return dependencies;
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

bool Checkable::IsReachable(DependencyType dt, Dependency::Ptr *failedDependency, int rstack) const
{
	/* Anything greater than 256 causes recursion bus errors. */
	int limit = 256;

	if (rstack > limit) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << limit << ") for checkable '" << GetName() << "': Dependency failed.";

		return false;
	}

	for (const Checkable::Ptr& checkable : GetParents()) {
		if (!checkable->IsReachable(dt, failedDependency, rstack + 1))
			return false;
	}

	/* implicit dependency on host if this is a service */
	const auto *service = dynamic_cast<const Service *>(this);
	if (service && (dt == DependencyState || dt == DependencyNotification)) {
		Host::Ptr host = service->GetHost();

		if (host && host->GetState() != HostUp && host->GetStateType() == StateTypeHard) {
			if (failedDependency)
				*failedDependency = nullptr;

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

				if (failedDependency)
					*failedDependency = dep;

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

		if (failedDependency)
			*failedDependency = violator->second;

		return false;
	}

	if (failedDependency)
		*failedDependency = nullptr;

	return true;
}

/**
 * Checks whether the last check result of this Checkable affects its child dependencies.
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

std::set<Checkable::Ptr> Checkable::GetAllChildren() const
{
	std::set<Checkable::Ptr> children = GetChildren();

	GetAllChildrenInternal(children, 0);

	return children;
}

void Checkable::GetAllChildrenInternal(std::set<Checkable::Ptr>& children, int level) const
{
	if (level > 32)
		return;

	std::set<Checkable::Ptr> localChildren;

	for (const Checkable::Ptr& checkable : children) {
		std::set<Checkable::Ptr> cChildren = checkable->GetChildren();

		if (!cChildren.empty()) {
			GetAllChildrenInternal(cChildren, level + 1);
			localChildren.insert(cChildren.begin(), cChildren.end());
		}

		localChildren.insert(checkable);
	}

	children.insert(localChildren.begin(), localChildren.end());
}
