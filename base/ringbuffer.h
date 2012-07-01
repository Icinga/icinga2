#ifndef RINGBUFFER_H
#define RINGBUFFER_H

namespace icinga
{

class I2_BASE_API Ringbuffer
{
public:
	Ringbuffer(int slots);

	int GetLength(void) const;
	void InsertValue(long tv, int num);
	int GetValues(long span) const;

private:
	vector<int> m_Slots;
	int m_Offset;
};

}

#endif /* RINGBUFFER_H */
