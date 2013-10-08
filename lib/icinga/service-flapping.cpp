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

#include "icinga/service.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include "base/convert.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

#define FLAPPING_INTERVAL (30 * 60)

double Service::GetFlappingCurrent(void) const
{
	if (m_FlappingPositive + m_FlappingNegative <= 0)
		return 0;

	return 100 * m_FlappingPositive / (m_FlappingPositive + m_FlappingNegative);
}

double Service::GetFlappingThreshold(void) const
{
	if (m_FlappingThreshold.IsEmpty())
		return 30;
	else
		return m_FlappingThreshold;
}

bool Service::GetEnableFlapping(void) const
{
	if (m_EnableFlapping.IsEmpty())
		return true;
	else
		return m_EnableFlapping;

}

void Service::SetEnableFlapping(bool enabled, const String& authority)
{
	m_EnableFlapping = enabled;

	OnFlappingChanged(GetSelf(), enabled ? FlappingEnabled : FlappingDisabled);
	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnEnableFlappingChanged), GetSelf(), enabled, authority));
}

void Service::UpdateFlappingStatus(bool stateChange)
{
	double ts, now;
	long positive, negative;

	now = Utility::GetTime();

	if (m_FlappingLastChange.IsEmpty()) {
		ts = now;
		positive = 0;
		negative = 0;
	} else {
		ts = m_FlappingLastChange;
		positive = m_FlappingPositive;
		negative = m_FlappingNegative;
	}

	double diff = now - ts;

	if (positive + negative > FLAPPING_INTERVAL) {
		double pct = (positive + negative - FLAPPING_INTERVAL) / FLAPPING_INTERVAL;
		positive -= pct * positive;
		negative -= pct * negative;
	}

	if (stateChange)
		positive += diff;
	else
		negative += diff;

	if (positive < 0)
		positive = 0;

	if (negative < 0)
		negative = 0;

//	Log(LogDebug, "icinga", "Flapping counter for '" + GetName() + "' is positive=" + Convert::ToString(positive) + ", negative=" + Convert::ToString(negative));

	m_FlappingPositive = positive;
	m_FlappingNegative = negative;
	m_FlappingLastChange = now;
}

bool Service::IsFlapping(void) const
{
	if (!GetEnableFlapping() || !IcingaApplication::GetInstance()->GetEnableFlapping())
		return false;
	else
		return GetFlappingCurrent() > GetFlappingThreshold();
}
