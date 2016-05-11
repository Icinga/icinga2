/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "remote/zone.hpp"
#include "remote/apilistener.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/initialize.hpp"
#include "base/timer.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

static Timer::Ptr l_AuthorityTimer;

static bool ObjectNameLessComparer(const ConfigObject::Ptr& a, const ConfigObject::Ptr& b)
{
	return a->GetName() < b->GetName();
}

void ApiListener::UpdateObjectAuthority(void)
{
	Zone::Ptr my_zone = Zone::GetLocalZone();

	std::vector<Endpoint::Ptr> endpoints;
	Endpoint::Ptr my_endpoint;

	if (my_zone) {
		my_endpoint = Endpoint::GetLocalEndpoint();

		int num_total = 0;

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, my_zone->GetEndpoints()) {
			num_total++;

			if (endpoint != my_endpoint && !endpoint->GetConnected())
				continue;

			endpoints.push_back(endpoint);
		}

		double mainTime = Application::GetMainTime();

		if (num_total > 1 && endpoints.size() <= 1 && (mainTime == 0 || Utility::GetTime() - mainTime < 60))
			return;

		std::sort(endpoints.begin(), endpoints.end(), ObjectNameLessComparer);
	}

	BOOST_FOREACH(const ConfigType::Ptr& type, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, type->GetObjects()) {
			if (object->GetHAMode() != HARunOnce)
				continue;

			bool authority;

			if (!my_zone)
				authority = true;
			else
				authority = endpoints[Utility::SDBM(object->GetName()) % endpoints.size()] == my_endpoint;

			object->SetAuthority(authority);
		}
	}
}

static void StaticInitialize(void)
{
	l_AuthorityTimer = new Timer();
	l_AuthorityTimer->OnTimerExpired.connect(boost::bind(&ApiListener::UpdateObjectAuthority));
	l_AuthorityTimer->SetInterval(30);
	l_AuthorityTimer->Start();
}

INITIALIZE_ONCE(StaticInitialize);
