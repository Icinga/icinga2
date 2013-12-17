/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/scheduleddowntime.h"
#include "icinga/legacytimeperiod.h"
#include "base/timer.h"
#include "base/dynamictype.h"
#include "base/initialize.h"
#include "base/utility.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(ScheduledDowntime);

INITIALIZE_ONCE(&ScheduledDowntime::StaticInitialize);

static Timer::Ptr l_Timer;

void ScheduledDowntime::StaticInitialize(void)
{
	l_Timer = make_shared<Timer>();
	l_Timer->SetInterval(60);
	l_Timer->OnTimerExpired.connect(boost::bind(&ScheduledDowntime::TimerProc));
	l_Timer->Start();
}

void ScheduledDowntime::Start(void)
{
	DynamicObject::Start();

	CreateNextDowntime();
}

void ScheduledDowntime::TimerProc(void)
{
	BOOST_FOREACH(const ScheduledDowntime::Ptr& sd, DynamicType::GetObjects<ScheduledDowntime>()) {
		sd->CreateNextDowntime();
	}
}

Service::Ptr ScheduledDowntime::GetService(void) const
{
	Host::Ptr host = Host::GetByName(GetHostRaw());

	if (GetServiceRaw().IsEmpty())
		return host->GetCheckService();
	else
		return host->GetServiceByShortName(GetServiceRaw());
}

std::pair<double, double> ScheduledDowntime::FindNextSegment(void)
{
	time_t refts = Utility::GetTime();
	tm reference = Utility::LocalTime(refts);

	Log(LogDebug, "icinga", "Finding next scheduled downtime segment for time " + Convert::ToString(static_cast<long>(refts)));

	Dictionary::Ptr ranges = GetRanges();

	Array::Ptr segments = make_shared<Array>();

	Dictionary::Ptr bestSegment;
	double bestBegin;
	double now = Utility::GetTime();

	ObjectLock olock(ranges);
	BOOST_FOREACH(const Dictionary::Pair& kv, ranges) {
		Dictionary::Ptr segment = LegacyTimePeriod::FindNextSegment(kv.first, kv.second, &reference);

		if (!segment)
			continue;

		double begin = segment->Get("begin");

		if (begin < now)
			continue;

		if (!bestSegment || begin < bestBegin) {
			bestSegment = segment;
			bestBegin = begin;
		}
	}

	if (bestSegment)
		return std::make_pair(bestSegment->Get("begin"), bestSegment->Get("end"));
	else
		return std::make_pair(0, 0);
}

void ScheduledDowntime::CreateNextDowntime(void)
{
	Dictionary::Ptr downtimes = GetService()->GetDowntimes();

	{
		ObjectLock dlock(downtimes);
		BOOST_FOREACH(const Dictionary::Pair& kv, downtimes) {
			Downtime::Ptr downtime = kv.second;

			if (downtime->GetScheduledBy() != GetName() ||
			    downtime->GetStartTime() < Utility::GetTime())
				continue;

			/* We've found a downtime that is owned by us and that hasn't started yet - we're done. */
			return;
		}
	}

	std::pair<double, double> segment = FindNextSegment();

	if (segment.first == 0 && segment.second == 0) {
		tm reference = Utility::LocalTime(Utility::GetTime());
		reference.tm_mday++;
		reference.tm_hour = 0;
		reference.tm_min = 0;
		reference.tm_sec = 0;

		return;
	}

	GetService()->AddDowntime(GetAuthor(), GetComment(),
	    segment.first, segment.second,
	    GetFixed(), String(), GetDuration(), GetName());
}
