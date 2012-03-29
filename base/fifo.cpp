#include "i2-base.h"

using namespace icinga;

FIFO::FIFO(void)
{
	m_Buffer = NULL;
	m_DataSize = 0;
	m_AllocSize = 0;
	m_Offset = 0;
}

FIFO::~FIFO(void)
{
	Memory::Free(m_Buffer);
}

void FIFO::ResizeBuffer(size_t newSize)
{
	if (m_AllocSize >= newSize)
		return;

	newSize = (newSize / FIFO::BlockSize + 1) * FIFO::BlockSize;

	m_Buffer = (char *)Memory::Reallocate(m_Buffer, newSize);
	m_AllocSize = newSize;
}

void FIFO::Optimize(void)
{
	//char *newBuffer;

	if (m_DataSize < m_Offset) {
		memcpy(m_Buffer, m_Buffer + m_Offset, m_DataSize);
		m_Offset = 0;

		return;
	}

	/*newBuffer = (char *)ResizeBuffer(NULL, 0, m_BufferSize - m_Offset);

	if (newBuffer == NULL)
		return;

	memcpy(newBuffer, m_Buffer + m_Offset, m_BufferSize - m_Offset);

	free(m_Buffer);
	m_Buffer = newBuffer;
	m_BufferSize -= m_Offset;
	m_Offset = 0;*/
}

size_t FIFO::GetSize(void) const
{
	return m_DataSize;
}

const void *FIFO::GetReadBuffer(void) const
{
	return m_Buffer + m_Offset;
}

size_t FIFO::Read(void *buffer, size_t count)
{
	count = (count <= m_DataSize) ? count : m_DataSize;

	if (buffer != NULL)
		memcpy(buffer, m_Buffer + m_Offset, count);

	m_DataSize -= count;
	m_Offset += count;

	Optimize();

	return count;
}

void *FIFO::GetWriteBuffer(size_t *count)
{
	ResizeBuffer(m_Offset + m_DataSize + *count);
	*count = m_AllocSize - m_Offset - m_DataSize;

	return m_Buffer + m_Offset + m_DataSize;
}

size_t FIFO::Write(const void *buffer, size_t count)
{
	if (buffer != NULL) {
		size_t bufferSize = count;
		void *target_buffer = GetWriteBuffer(&bufferSize);
		memcpy(target_buffer, buffer, count);
	}

	m_DataSize += count;

	return count;
}
