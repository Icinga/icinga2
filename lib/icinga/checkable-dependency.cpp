/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/service.hpp"
#include "icinga/dependency.hpp"
#include "base/logger.hpp"

using namespace icinga;

void Checkable::AddDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_Dependencies.insert(dep);
}

void Checkable::RemoveDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_Dependencies.erase(dep);
}

std::vector<Dependency::Ptr> Checkable::GetDependencies() const
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	return std::vector<Dependency::Ptr>(m_Dependencies.begin(), m_Dependencies.end());
}

void Checkable::AddReverseDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_ReverseDependencies.insert(dep);
}

void Checkable::RemoveReverseDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_ReverseDependencies.erase(dep);
}

std::vector<Dependency::Ptr> Checkable::GetReverseDependencies() const
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
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
		double recovery_time = service->GetRecoveryTime();
		if (recovery_time == 0.0 && host)
			recovery_time = host->GetRecoveryTime();
		double now = Utility::GetTime();

		if (host && ((host->GetState() != HostUp && host->GetStateType() == StateTypeHard) || (host->GetState() == HostUp && now - host->GetLastStateChange() < recovery_time))) {
			if (failedDependency)
				*failedDependency = nullptr;

			return false;
		}
	}

	auto deps = GetDependencies();

	int countDeps = deps.size();
	int countFailed = 0;

	for (const Dependency::Ptr& dep : deps) {
		if (!dep->IsAvailable(dt)) {
			countFailed++;

			if (failedDependency)
				*failedDependency = dep;
		}
	}

	/* If there are dependencies, and all of them failed, mark as unreachable. */
	if (countDeps > 0 && countFailed == countDeps) {
		Log(LogDebug, "Checkable")
			<< "All dependencies have failed for checkable '" << GetName() << "': Marking as unreachable.";

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
