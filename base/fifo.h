#ifndef I2_FIFO_H
#define I2_FIFO_H

namespace icinga
{

class FIFO : public Object
{
private:
	char *m_Buffer;
	size_t m_DataSize;
	size_t m_AllocSize;
	size_t m_Offset;

	void ResizeBuffer(size_t newSize);
	void Optimize(void);

public:
	static const size_t BlockSize = 16 * 1024;

	typedef shared_ptr<FIFO> RefType;
	typedef weak_ptr<FIFO> WeakRefType;

	FIFO(void);
	~FIFO(void);

	size_t GetSize(void) const;

	const void *GetReadBuffer(void) const;
	void *GetWriteBuffer(size_t *count);

	size_t Read(void *buffer, size_t count);
	size_t Write(const void *buffer, size_t count);
};

}

#endif /* I2_FIFO_H */