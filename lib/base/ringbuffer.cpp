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

#include "base/ringbuffer.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include <algorithm>

using namespace icinga;

RingBuffer::RingBuffer(RingBuffer::SizeType slots)
	: Object(), m_Slots(slots, 0), m_TimeValue(0), m_InsertedValues(0)
{ }

RingBuffer::SizeType RingBuffer::GetLength() const
{
	ObjectLock olock(this);

	return m_Slots.size();
}

void RingBuffer::InsertValue(RingBuffer::SizeType tv, int num)
{
	ObjectLock olock(this);

	RingBuffer::SizeType offsetTarget = tv % m_Slots.size();

	if (m_TimeValue == 0)
		m_InsertedValues = 1;

	if (tv > m_TimeValue) {
		RingBuffer::SizeType offset = m_TimeValue % m_Slots.size();

		/* walk towards the target offset, resetting slots to 0 */
		while (offset != offsetTarget) {
			offset++;

			if (offset >= m_Slots.size())
				offset = 0;

			m_Slots[offset] = 0;

			if (m_TimeValue != 0 && m_InsertedValues < m_Slots.size())
				m_InsertedValues++;
		}

		m_TimeValue = tv;
	}

	m_Slots[offsetTarget] += num;
}

int RingBuffer::UpdateAndGetValues(RingBuffer::SizeType tv, RingBuffer::SizeType span)
{
	ObjectLock olock(this);

	InsertValue(tv, 0);

	if (span > m_Slots.size())
		span = m_Slots.size();

	int off = m_TimeValue % m_Slots.size();
	int sum = 0;
	while (span > 0) {
		sum += m_Slots[off];

		if (off == 0)
			off = m_Slots.size();

		off--;
		span--;
	}

	return sum;
}

double RingBuffer::CalculateRate(RingBuffer::SizeType tv, RingBuffer::SizeType span)
{
	ObjectLock olock(this);
	int sum = UpdateAndGetValues(tv, span);
	return sum / static_cast<double>(std::min(span, m_InsertedValues));
}
