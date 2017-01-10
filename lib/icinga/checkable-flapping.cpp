/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "icinga/checkable.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/utility.hpp"

using namespace icinga;

#define FLAPPING_INTERVAL (30 * 60)

double Checkable::GetFlappingCurrent(void) const
{
	if (GetFlappingPositive() + GetFlappingNegative() <= 0)
		return 0;

	return 100 * GetFlappingPositive() / (GetFlappingPositive() + GetFlappingNegative());
}

void Checkable::UpdateFlappingStatus(bool stateChange)
{
	double ts, now;
	long positive, negative;

	now = Utility::GetTime();

	ts = GetFlappingLastChange();
	positive = GetFlappingPositive();
	negative = GetFlappingNegative();

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

//	Log(LogDebug, "Checkable")
//	    << "Flapping counter for '" << GetName() << "' is positive=" << positive << ", negative=" << negative;

	SetFlappingLastChange(now);
	SetFlappingPositive(positive);
	SetFlappingNegative(negative);
}

bool Checkable::IsFlapping(void) const
{
	if (!GetEnableFlapping() || !IcingaApplication::GetInstance()->GetEnableFlapping())
		return false;
	else
		return GetFlappingCurrent() > GetFlappingThreshold();
}
