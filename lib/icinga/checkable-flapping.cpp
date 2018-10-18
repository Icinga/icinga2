/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

template<typename T>
struct Bitset
{
public:
	Bitset(T value)
		: m_Data(value)
	{ }

	void Modify(int index, bool bit)
	{
		if (bit)
			m_Data |= 1 << index;
		else
			m_Data &= ~(1 << index);
	}

	bool Get(int index) const
	{
		return m_Data & (1 << index);
	}

	T GetValue() const
	{
		return m_Data;
	}

private:
	T m_Data{0};
};

void Checkable::UpdateFlappingStatus(bool stateChange)
{
	Bitset<unsigned long> stateChangeBuf = GetFlappingBuffer();
	int oldestIndex = GetFlappingIndex();

	stateChangeBuf.Modify(oldestIndex, stateChange);
	oldestIndex = (oldestIndex + 1) % 20;

	double stateChanges = 0;

	/* Iterate over our state array and compute a weighted total */
	for (int i = 0; i < 20; i++) {
		if (stateChangeBuf.Get((oldestIndex + i) % 20))
			stateChanges += 0.8 + (0.02 * i);
	}

	double flappingValue = 100.0 * stateChanges / 20.0;

	bool flapping;

	if (GetFlapping())
		flapping = flappingValue > GetFlappingThresholdLow();
	else
		flapping = flappingValue > GetFlappingThresholdHigh();

	SetFlappingBuffer(stateChangeBuf.GetValue());
	SetFlappingIndex(oldestIndex);
	SetFlappingCurrent(flappingValue);
	SetFlapping(flapping, true);

	if (flapping != GetFlapping())
		SetFlappingLastChange(Utility::GetTime());
}

bool Checkable::IsFlapping() const
{
	if (!GetEnableFlapping() || !IcingaApplication::GetInstance()->GetEnableFlapping())
		return false;
	else
		return GetFlapping();
}
