/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "icinga/hostgroup.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/timer.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static boost::mutex l_Mutex;
static std::map<String, std::vector<Host::WeakPtr> > l_MembersCache;
static bool l_MembersCacheNeedsUpdate = false;
static Timer::Ptr l_MembersCacheTimer;

REGISTER_TYPE(HostGroup);

HostGroup::HostGroup(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("notes_url", Attribute_Config, &m_NotesUrl);
	RegisterAttribute("action_url", Attribute_Config, &m_ActionUrl);
}

HostGroup::~HostGroup(void)
{
	InvalidateMembersCache();
}

/**
 * @threadsafety Always.
 */
void HostGroup::OnRegistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	InvalidateMembersCache();
}

/**
 * @threadsafety Always.
 */
String HostGroup::GetDisplayName(void) const
{
	if (!m_DisplayName.IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

/**
 * @threadsafety Always.
 */
String HostGroup::GetNotesUrl(void) const
{
	return m_NotesUrl;
}

/**
 * @threadsafety Always.
 */
String HostGroup::GetActionUrl(void) const
{
	return m_ActionUrl;
}

/**
 * @threadsafety Always.
 */
HostGroup::Ptr HostGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("HostGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(std::invalid_argument("HostGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<HostGroup>(configObject);
}

/**
 * @threadsafety Always.
 */
std::set<Host::Ptr> HostGroup::GetMembers(void) const
{
	std::set<Host::Ptr> hosts;

	{
		boost::mutex::scoped_lock lock(l_Mutex);

		BOOST_FOREACH(const Host::WeakPtr& whost, l_MembersCache[GetName()]) {
			Host::Ptr host = whost.lock();

			if (!host)
				continue;

			hosts.insert(host);
		}
	}

	return hosts;
}

/**
 * @threadsafety Always.
 */
void HostGroup::InvalidateMembersCache(void)
{
	boost::mutex::scoped_lock lock(l_Mutex);

	if (l_MembersCacheNeedsUpdate)
		return; /* Someone else has already requested a refresh. */

	if (!l_MembersCacheTimer) {
		l_MembersCacheTimer = boost::make_shared<Timer>();
		l_MembersCacheTimer->SetInterval(0.5);
		l_MembersCacheTimer->OnTimerExpired.connect(boost::bind(&HostGroup::RefreshMembersCache));
		l_MembersCacheTimer->Start();
	}

	l_MembersCacheNeedsUpdate = true;
}

/**
 * @threadsafety Always.
 */
void HostGroup::RefreshMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_Mutex);

		if (!l_MembersCacheNeedsUpdate)
			return;

		l_MembersCacheNeedsUpdate = false;
	}

	Log(LogDebug, "icinga", "Updating HostGroup members cache.");

	std::map<String, std::vector<Host::WeakPtr> > newMembersCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		const Host::Ptr& host = static_pointer_cast<Host>(object);

		Array::Ptr groups;
		groups = host->GetGroups();

		if (groups) {
			ObjectLock mlock(groups);
			BOOST_FOREACH(const Value& group, groups) {
				newMembersCache[group].push_back(host);
			}
		}
	}

	boost::mutex::scoped_lock lock(l_Mutex);
	l_MembersCache.swap(newMembersCache);
}
