/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/dependency.hpp"
#include "base/logger.hpp"
#include <unordered_map>

using namespace icinga;

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
 * Note, this function performs a recursive call chain traversing all the children of the current Checkable
 * up to a certain limit (256). When that limit is reached, it will log a warning message and abort the operation.
 * Thus, the returned number may not reflect the actual total number of children involved in the dependency chain.
 *
 * In fact, there is already a method GetAllChildren() that does kind of same thing, but it involves too much
 * overhead, copying and passing around sets, which is not necessary for this simple operation (just count them).
 *
 * @return int - Returns the total number of all the children of the current Checkable.
 */
int Checkable::GetAllChildrenCount(int rstack) const
{
	if (rstack > 256) {
		Log(LogWarning, "Checkable")
			<< "Too many nested dependencies (>" << 256 << ") for checkable '" << GetName() << "': aborting count.";

		// The limit from GetAllChildrenInternal() doesn't seem to make sense, and appears to be
		// some random number. So, this limit is set to 256 to match the limit in IsReachable().
		return 0;
	}

	int count = 0;
	// Actually, incrementing the rstack should be done once outside the loop here, but since IsReachable()
	// applies the limit the exact same way, it should be fine to limit the sum of calls to 256 here as well.
	for (auto& dependency: GetReverseDependencies()) {
		count += dependency->GetChild()->GetAllChildrenCount(rstack + 1) + 1;
	}

	return count;
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
