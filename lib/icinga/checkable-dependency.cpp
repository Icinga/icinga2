/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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

std::vector<Dependency::Ptr> Checkable::GetDependencies(void) const
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

std::vector<Dependency::Ptr> Checkable::GetReverseDependencies(void) const
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	return std::vector<Dependency::Ptr>(m_ReverseDependencies.begin(), m_ReverseDependencies.end());
}

bool Checkable::IsReachable(DependencyType dt, Dependency::Ptr *failedDependency, int rstack) const
{
	if (rstack > 20) {
		Log(LogWarning, "Checkable")
		    << "Too many nested dependencies for service '" << GetName() << "': Dependency failed.";

		return false;
	}

	for (const Checkable::Ptr& checkable : GetParents()) {
		if (!checkable->IsReachable(dt, failedDependency, rstack + 1))
			return false;
	}

	/* implicit dependency on host if this is a service */
	const Service *service = dynamic_cast<const Service *>(this);
	if (service && (dt == DependencyState || dt == DependencyNotification)) {
		Host::Ptr host = service->GetHost();

		if (host && host->GetState() != HostUp && host->GetStateType() == StateTypeHard) {
			if (failedDependency)
				*failedDependency = Dependency::Ptr();

			return false;
		}
	}

	for (const Dependency::Ptr& dep : GetDependencies()) {
		if (!dep->IsAvailable(dt)) {
			if (failedDependency)
				*failedDependency = dep;

			return false;
		}
	}

	if (failedDependency)
		*failedDependency = Dependency::Ptr();

	return true;
}

std::set<Checkable::Ptr> Checkable::GetParents(void) const
{
	std::set<Checkable::Ptr> parents;

	for (const Dependency::Ptr& dep : GetDependencies()) {
		Checkable::Ptr parent = dep->GetParent();

		if (parent && parent.get() != this)
			parents.insert(parent);
	}

	return parents;
}

std::set<Checkable::Ptr> Checkable::GetChildren(void) const
{
	std::set<Checkable::Ptr> parents;

	for (const Dependency::Ptr& dep : GetReverseDependencies()) {
		Checkable::Ptr service = dep->GetChild();

		if (service && service.get() != this)
			parents.insert(service);
	}

	return parents;
}

std::set<Checkable::Ptr> Checkable::GetAllChildren(void) const
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
