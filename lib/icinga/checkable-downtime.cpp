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

#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

void Checkable::RemoveAllDowntimes(void)
{
	BOOST_FOREACH(const Downtime::Ptr& downtime, GetDowntimes()) {
		Downtime::RemoveDowntime(downtime->GetName(), true, true);
	}
}

void Checkable::TriggerDowntimes(void)
{
	BOOST_FOREACH(const Downtime::Ptr& downtime, GetDowntimes()) {
		downtime->TriggerDowntime();
	}
}

bool Checkable::IsInDowntime(void) const
{
	BOOST_FOREACH(const Downtime::Ptr& downtime, GetDowntimes()) {
		if (downtime->IsActive())
			return true;
	}

	return false;
}

int Checkable::GetDowntimeDepth(void) const
{
	int downtime_depth = 0;

	BOOST_FOREACH(const Downtime::Ptr& downtime, GetDowntimes()) {
		if (downtime->IsActive())
			downtime_depth++;
	}

	return downtime_depth;
}

std::set<Downtime::Ptr> Checkable::GetDowntimes(void) const
{
	boost::mutex::scoped_lock lock(m_DowntimeMutex);
	return m_Downtimes;
}

void Checkable::RegisterDowntime(const Downtime::Ptr& downtime)
{
	boost::mutex::scoped_lock lock(m_DowntimeMutex);
	m_Downtimes.insert(downtime);
}

void Checkable::UnregisterDowntime(const Downtime::Ptr& downtime)
{
	boost::mutex::scoped_lock lock(m_DowntimeMutex);
	m_Downtimes.erase(downtime);
}
