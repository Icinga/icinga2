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

#include "i2-icinga.h"
#include "icinga/servicegroup.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static boost::mutex l_Mutex;
static std::map<String, std::vector<Service::WeakPtr> > l_MembersCache;
static bool l_MembersCacheNeedsUpdate = false;
static Timer::Ptr l_MembersCacheTimer;
boost::signals2::signal<void (void)> ServiceGroup::OnMembersChanged;

REGISTER_TYPE(ServiceGroup);

ServiceGroup::ServiceGroup(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
}

ServiceGroup::~ServiceGroup(void)
{
	InvalidateMembersCache();
}

void ServiceGroup::OnRegistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	InvalidateMembersCache();
}

String ServiceGroup::GetDisplayName(void) const
{
	if (!m_DisplayName.Get().IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

ServiceGroup::Ptr ServiceGroup::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("ServiceGroup", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(std::invalid_argument("ServiceGroup '" + name + "' does not exist."));

	return dynamic_pointer_cast<ServiceGroup>(configObject);
}

std::set<Service::Ptr> ServiceGroup::GetMembers(void) const
{
	std::set<Service::Ptr> services;

	{
		boost::mutex::scoped_lock lock(l_Mutex);

		BOOST_FOREACH(const Service::WeakPtr& wservice, l_MembersCache[GetName()]) {
			Service::Ptr service = wservice.lock();

			if (!service)
				continue;

			services.insert(service);
		}
	}

	return services;
}

void ServiceGroup::InvalidateMembersCache(void)
{
	boost::mutex::scoped_lock lock(l_Mutex);

	if (l_MembersCacheNeedsUpdate)
		return; /* Someone else has already requested a refresh. */

	if (!l_MembersCacheTimer) {
		l_MembersCacheTimer = boost::make_shared<Timer>();
		l_MembersCacheTimer->SetInterval(0.5);
		l_MembersCacheTimer->OnTimerExpired.connect(boost::bind(&ServiceGroup::RefreshMembersCache));
		l_MembersCacheTimer->Start();
	}

	l_MembersCacheNeedsUpdate = true;
}

void ServiceGroup::RefreshMembersCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_Mutex);

		if (!l_MembersCacheNeedsUpdate)
			return;

		l_MembersCacheNeedsUpdate = false;
	}

	Log(LogDebug, "icinga", "Updating ServiceGroup members cache.");

	std::map<String, std::vector<Service::WeakPtr> > newMembersCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);

		Array::Ptr groups = service->GetGroups();

		if (groups) {
			ObjectLock mlock(groups);
			BOOST_FOREACH(const Value& group, groups) {
				newMembersCache[group].push_back(service);
			}
		}
	}

	{
		boost::mutex::scoped_lock lock(l_Mutex);
		l_MembersCache.swap(newMembersCache);
	}

	OnMembersChanged();
}
