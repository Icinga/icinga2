/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/ringbuffer.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include <algorithm>

using namespace icinga;

RingBuffer::RingBuffer(RingBuffer::SizeType slots)
	: m_Slots(slots, 0), m_TimeValue(0), m_InsertedValues(0)
{ }

RingBuffer::SizeType RingBuffer::GetLength() const
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	return m_Slots.size();
}

void RingBuffer::InsertValue(RingBuffer::SizeType tv, int num)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	InsertValueUnlocked(tv, num);
}

void RingBuffer::InsertValueUnlocked(RingBuffer::SizeType tv, int num)
{
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
	std::unique_lock<std::mutex> lock(m_Mutex);

	return UpdateAndGetValuesUnlocked(tv, span);
}

int RingBuffer::UpdateAndGetValuesUnlocked(RingBuffer::SizeType tv, RingBuffer::SizeType span)
{
	InsertValueUnlocked(tv, 0);

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
	std::unique_lock<std::mutex> lock(m_Mutex);

	int sum = UpdateAndGetValuesUnlocked(tv, span);
	return sum / static_cast<double>(std::min(span, m_InsertedValues));
}
