#ifndef RINGBUFFER_H
#define RINGBUFFER_H

namespace icinga
{

class I2_BASE_API RingBuffer
{
public:
	RingBuffer(long slots);

	long GetLength(void) const;
	void InsertValue(long tv, int num);
	int GetValues(long span) const;

private:
	vector<int> m_Slots;
	vector<int>::size_type m_Offset;
};

}

#endif /* RINGBUFFER_H */
