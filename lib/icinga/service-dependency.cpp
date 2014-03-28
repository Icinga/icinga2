/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/service.h"
#include "icinga/dependency.h"
#include "config/configitembuilder.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include "base/convert.h"
#include <boost/foreach.hpp>

using namespace icinga;

void Service::AddDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_Dependencies.insert(dep);
}

void Service::RemoveDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_Dependencies.erase(dep);
}

std::set<Dependency::Ptr> Service::GetDependencies(void) const
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	return m_Dependencies;
}

void Service::AddReverseDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_ReverseDependencies.insert(dep);
}

void Service::RemoveReverseDependency(const Dependency::Ptr& dep)
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	m_ReverseDependencies.erase(dep);
}

std::set<Dependency::Ptr> Service::GetReverseDependencies(void) const
{
	boost::mutex::scoped_lock lock(m_DependencyMutex);
	return m_ReverseDependencies;
}

bool Service::IsReachable(DependencyType dt, Dependency::Ptr *failedDependency, int rstack) const
{
	if (rstack > 20) {
		Log(LogWarning, "icinga", "Too many nested dependencies for service '" + GetName() + "': Dependency failed.");

		return false;
	}

	BOOST_FOREACH(const Service::Ptr& service, GetParentServices()) {
		if (!service->IsReachable(dt, failedDependency, rstack + 1))
			return false;
	}

	/* implicit dependency on host's check service */
	if (dt == DependencyState || dt == DependencyNotification) {
		Service::Ptr hc = GetHost()->GetCheckService();

		if (hc && hc->GetState() == StateCritical && hc->GetStateType() == StateTypeHard) {
			if (failedDependency)
				*failedDependency = Dependency::Ptr();

			return false;
		}
	}

	BOOST_FOREACH(const Dependency::Ptr& dep, GetDependencies()) {
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

std::set<Host::Ptr> Service::GetParentHosts(void) const
{
	std::set<Host::Ptr> result;

	BOOST_FOREACH(const Service::Ptr& svc, GetParentServices())
		result.insert(svc->GetHost());

	return result;
}

std::set<Host::Ptr> Service::GetChildHosts(void) const
{
	std::set<Host::Ptr> result;

	BOOST_FOREACH(const Service::Ptr& svc, GetChildServices())
		result.insert(svc->GetHost());

	return result;
}

std::set<Service::Ptr> Service::GetParentServices(void) const
{
	std::set<Service::Ptr> parents;

	BOOST_FOREACH(const Dependency::Ptr& dep, GetDependencies()) {
		Service::Ptr service = dep->GetParentService();

		if (service)
			parents.insert(service);
	}

	return parents;
}

std::set<Service::Ptr> Service::GetChildServices(void) const
{
	std::set<Service::Ptr> parents;

	BOOST_FOREACH(const Dependency::Ptr& dep, GetReverseDependencies()) {
		Service::Ptr service = dep->GetChildService();

		if (service)
			parents.insert(service);
	}

	return parents;
}

