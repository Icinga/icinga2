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

void Service::UpdateSlaveDependencies(void)
{
	/*
	 * pass == 0 -> steal host's dependency definitions
	 * pass == 1 -> service's dependencies
	 */
	for (int pass = 0; pass < 2; pass++) {
		/* Service dependency descs */
		Dictionary::Ptr descs;

		if (pass == 0 && !IsHostCheck())
			continue;

		if (pass == 0)
			descs = GetHost()->GetDependencyDescriptions();
		else
			descs = GetDependencyDescriptions();

		if (!descs || descs->GetLength() == 0)
			continue;

		ConfigItem::Ptr item;

		if (pass == 0)
			item = ConfigItem::GetObject("Host", GetHost()->GetName());
		else
			item = ConfigItem::GetObject("Service", GetName());

		ObjectLock olock(descs);

		BOOST_FOREACH(const Dictionary::Pair& kv, descs) {
			std::ostringstream namebuf;
			namebuf << GetName() << "!" << kv.first;
			String name = namebuf.str();

			std::vector<String> path;
			path.push_back("dependencies");
			path.push_back(kv.first);
	
			AExpression::Ptr exprl;

			{
				ObjectLock ilock(item);

				exprl = item->GetLinkedExpressionList();
			}

			DebugInfo di;
			exprl->FindDebugInfoPath(path, di);

			if (di.Path.IsEmpty())
				di = item->GetDebugInfo();

			ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(di);
			builder->SetType("Dependency");
			builder->SetName(name);
			builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet, "child_host", make_shared<AExpression>(&AExpression::OpLiteral, GetHost()->GetName(), di), di));
			builder->AddExpression(make_shared<AExpression>(&AExpression::OpSet, "child_service", make_shared<AExpression>(&AExpression::OpLiteral, GetShortName(), di), di));

			Dictionary::Ptr dependency = kv.second;

			Array::Ptr templates = dependency->Get("templates");

			if (templates) {
				ObjectLock tlock(templates);

				BOOST_FOREACH(const Value& tmpl, templates) {
					builder->AddParent(tmpl);
				}
			}

			/* Clone attributes from the scheduled downtime expression list. */
			Array::Ptr sd_exprl = make_shared<Array>();
			exprl->ExtractPath(path, sd_exprl);

			builder->AddExpression(make_shared<AExpression>(&AExpression::OpDict, sd_exprl, true, di));

			builder->SetScope(item->GetScope());

			ConfigItem::Ptr dependencyItem = builder->Compile();
			dependencyItem->Register();
			DynamicObject::Ptr dobj = dependencyItem->Commit();
			dobj->OnConfigLoaded();
		}
	}
}
