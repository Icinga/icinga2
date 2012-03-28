#ifndef I2_FIFO_H
#define I2_FIFO_H

namespace icinga
{

class FIFO : public Object
{
private:
	static const size_t BlockSize = 16 * 1024;
	char *m_Buffer;
	size_t m_BufferSize;
	size_t m_Offset;

	char *ResizeBuffer(char *buffer, size_t oldSize, size_t newSize);
	void Optimize(void);

public:
	typedef shared_ptr<FIFO> RefType;
	typedef weak_ptr<FIFO> WeakRefType;

	FIFO(void);
	~FIFO(void);

	size_t GetSize(void) const;

	const void *Peek(void) const;
	size_t Read(void *buffer, size_t count);
	size_t Write(const void *buffer, size_t count);
};

}

#endif /* I2_FIFO_H */