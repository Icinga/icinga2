#include "i2-base.h"

using namespace icinga;

Ringbuffer::Ringbuffer(int slots)
	: m_Slots(slots, 0), m_Offset(0)
{ }

int Ringbuffer::GetLength(void) const
{
	return m_Slots.size();
}

void Ringbuffer::InsertValue(long tv, int num)
{
	int offsetTarget = tv % m_Slots.size();

	/* walk towards the target offset, resetting slots to 0 */
	while (m_Offset != offsetTarget) {
		m_Offset++;

		if (m_Offset >= m_Slots.size())
			m_Offset = 0;

		m_Slots[m_Offset] = 0;
	}

	m_Slots[m_Offset] += num;
}

int Ringbuffer::GetValues(long span) const
{
	if (span > m_Slots.size())
		span = m_Slots.size();

	int off = m_Offset;
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
