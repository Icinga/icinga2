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

#include "remote/zone.h"
#include "remote/apilistener.h"
#include "base/application.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/initialize.h"
#include "base/timer.h"
#include "base/logger_fwd.h"
#include "base/exception.h"

using namespace icinga;

static Timer::Ptr l_AuthorityTimer;

static bool ObjectNameLessComparer(const DynamicObject::Ptr& a, const DynamicObject::Ptr& b)
{
	return a->GetName() < b->GetName();
}

static void AuthorityTimerHandler(void)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener || !listener->IsActive())
		return;

	Zone::Ptr my_zone = Zone::GetLocalZone();
	Endpoint::Ptr my_endpoint = Endpoint::GetLocalEndpoint();

	std::vector<Endpoint::Ptr> endpoints;
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, my_zone->GetEndpoints()) {
		if (!endpoint->IsConnected() && endpoint != my_endpoint)
			continue;

		endpoints.push_back(endpoint);
	}

	std::sort(endpoints.begin(), endpoints.end(), ObjectNameLessComparer);

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			Endpoint::Ptr endpoint = endpoints[Utility::SDBM(object->GetName()) % endpoints.size()];

			if (endpoint == my_endpoint)
				object->Resume();
			else
				object->Pause();
		}
	}
}

static void StaticInitialize(void)
{
	l_AuthorityTimer = make_shared<Timer>();
	l_AuthorityTimer->OnTimerExpired.connect(boost::bind(&AuthorityTimerHandler));
	l_AuthorityTimer->SetInterval(30);
	l_AuthorityTimer->Start();
}

INITIALIZE_ONCE(StaticInitialize);
