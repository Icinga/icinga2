/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <vector>
#include <mutex>

namespace icinga
{

/**
 * A ring buffer that holds a pre-defined number of integers.
 *
 * @ingroup base
 */
class RingBuffer final
{
public:
	DECLARE_PTR_TYPEDEFS(RingBuffer);

	typedef std::vector<int>::size_type SizeType;

	RingBuffer(SizeType slots);

	SizeType GetLength() const;
	void InsertValue(SizeType tv, int num);
	int UpdateAndGetValues(SizeType tv, SizeType span);
	double CalculateRate(SizeType tv, SizeType span);

private:
	mutable std::mutex m_Mutex;
	std::vector<int> m_Slots;
	SizeType m_TimeValue;
	SizeType m_InsertedValues;

	void InsertValueUnlocked(SizeType tv, int num);
	int UpdateAndGetValuesUnlocked(SizeType tv, SizeType span);
};

}

#endif /* RINGBUFFER_H */
