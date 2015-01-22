/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/hostgroup.hpp"
#include "config/objectrule.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/context.hpp"
#include "base/workqueue.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(HostGroup);

INITIALIZE_ONCE(&HostGroup::RegisterObjectRuleHandler);

void HostGroup::RegisterObjectRuleHandler(void)
{
	ObjectRule::RegisterType("HostGroup", &HostGroup::EvaluateObjectRules);
}

bool HostGroup::EvaluateObjectRuleOne(const Host::Ptr& host, const ObjectRule& rule)
{
	DebugInfo di = rule.GetDebugInfo();

	std::ostringstream msgbuf;
	msgbuf << "Evaluating 'object' rule (" << di << ")";
	CONTEXT(msgbuf.str());

	Dictionary::Ptr locals = new Dictionary();
	locals->Set("__parent", rule.GetScope());
	locals->Set("host", host);

	if (!rule.EvaluateFilter(locals))
		return false;

	Log(LogDebug, "HostGroup")
	    << "Assigning membership for group '" << rule.GetName() << "' to host '" << host->GetName() << "' for rule " << di;

	String group_name = rule.GetName();
	HostGroup::Ptr group = HostGroup::GetByName(group_name);

	if (!group) {
		Log(LogCritical, "HostGroup")
		    << "Invalid membership assignment. Group '" << group_name << "' does not exist.";
		return false;
	}

	/* assign host group membership */
	group->ResolveGroupMembership(host, true);

	return true;
}

void HostGroup::EvaluateObjectRule(const ObjectRule& rule)
{
	BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjectsByType<Host>()) {
		CONTEXT("Evaluating group membership in '" + rule.GetName() + "' for host '" + host->GetName() + "'");

		EvaluateObjectRuleOne(host, rule);
	}
}

void HostGroup::EvaluateObjectRules(const std::vector<ObjectRule>& rules)
{
	ParallelWorkQueue upq;

	BOOST_FOREACH(const ObjectRule& rule, rules) {
		upq.Enqueue(boost::bind(HostGroup::EvaluateObjectRule, boost::cref(rule)));
	}

	upq.Join();
}

std::set<Host::Ptr> HostGroup::GetMembers(void) const
{
	boost::mutex::scoped_lock lock(m_HostGroupMutex);
	return m_Members;
}

void HostGroup::AddMember(const Host::Ptr& host)
{
	host->AddGroup(GetName());

	boost::mutex::scoped_lock lock(m_HostGroupMutex);
	m_Members.insert(host);
}

void HostGroup::RemoveMember(const Host::Ptr& host)
{
	boost::mutex::scoped_lock lock(m_HostGroupMutex);
	m_Members.erase(host);
}

bool HostGroup::ResolveGroupMembership(const Host::Ptr& host, bool add, int rstack) {

	if (add && rstack > 20) {
		Log(LogWarning, "HostGroup")
		    << "Too many nested groups for group '" << GetName() << "': Host '"
		    << host->GetName() << "' membership assignment failed.";

		return false;
	}

	Array::Ptr groups = GetGroups();

	if (groups && groups->GetLength() > 0) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr group = HostGroup::GetByName(name);

			if (group && !group->ResolveGroupMembership(host, add, rstack + 1))
				return false;
		}
	}

	if (add)
		AddMember(host);
	else
		RemoveMember(host);

	return true;
}
