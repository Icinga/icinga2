/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include <bitset>
#include "icinga/checkable.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/utility.hpp"

using namespace icinga;

void Checkable::UpdateFlappingStatus(bool stateChange)
{
/* TODO: Add support for Windows satellites/masters. */
#ifndef _WIN32 /* _WIN32 */
	std::bitset<20> stateChangeBuf = GetFlappingBuffer();
	int oldestIndex = GetFlappingIndex();

	stateChangeBuf[oldestIndex] = stateChange;
	oldestIndex = (oldestIndex + 1) % 20;

	double stateChanges = 0;

	/* Iterate over our state array and compute a weighted total */
	for (int i = 0; i < 20; i++) {
		if (stateChangeBuf[(oldestIndex + i) % 20])
			stateChanges += 0.8 + (0.02 * i);
	}

	double flappingValue = 100.0 * stateChanges / 20.0;

	bool flapping;

	if (GetFlapping())
		flapping = flappingValue > GetFlappingThresholdLow();
	else
		flapping = flappingValue > GetFlappingThresholdHigh();

	SetFlappingBuffer(stateChangeBuf.to_ulong());
	SetFlappingIndex(oldestIndex);
	SetFlappingCurrent(flappingValue);
	SetFlapping(flapping, true);

	if (flapping != GetFlapping())
		SetFlappingLastChange(Utility::GetTime());

#endif /* _WIN32 */
}

bool Checkable::IsFlapping() const
{
	if (!GetEnableFlapping() || !IcingaApplication::GetInstance()->GetEnableFlapping())
		return false;
	else
		return GetFlapping();
}
