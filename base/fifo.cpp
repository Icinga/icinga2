#include "i2-base.h"

using namespace icinga;

FIFO::FIFO(void)
{
	m_Buffer = NULL;
	m_BufferSize = 0;
	m_Offset = 0;
}

FIFO::~FIFO(void)
{
	Memory::Free(m_Buffer);
}

char *FIFO::ResizeBuffer(char *buffer, size_t oldSize, size_t newSize)
{
	if (oldSize != 0)
		oldSize += FIFO::BlockSize - (oldSize % FIFO::BlockSize);

	size_t ceilNewSize = newSize + FIFO::BlockSize - (newSize % FIFO::BlockSize);
	size_t oldBlocks = oldSize / FIFO::BlockSize;
	size_t newBlocks = ceilNewSize / FIFO::BlockSize;

	if (oldBlocks == newBlocks)
		return buffer;

	if (newSize == 0) {
		Memory::Free(buffer);
		return NULL;
	} else {
		return (char *)Memory::Reallocate(buffer, newBlocks * FIFO::BlockSize);
	}
}

void FIFO::Optimize(void)
{
	char *newBuffer;

	if (m_Offset == 0 || m_Offset < m_BufferSize / 5)
		return;

	if (m_BufferSize - m_Offset == 0) {
		free(m_Buffer);

		m_Buffer = 0;
		m_BufferSize = 0;
		m_Offset = 0;

		return;
	}

	newBuffer = (char *)ResizeBuffer(NULL, 0, m_BufferSize - m_Offset);

	if (newBuffer == NULL)
		return;

	memcpy(newBuffer, m_Buffer + m_Offset, m_BufferSize - m_Offset);

	free(m_Buffer);
	m_Buffer = newBuffer;
	m_BufferSize -= m_Offset;
	m_Offset = 0;
}

size_t FIFO::GetSize(void) const
{
	return m_BufferSize - m_Offset;
}

const void *FIFO::GetReadBuffer(void) const
{
	return m_Buffer + m_Offset;
}

size_t FIFO::Read(void *buffer, size_t count)
{
	count = (count <= m_BufferSize) ? count : m_BufferSize;

	if (buffer != NULL)
		memcpy(buffer, m_Buffer + m_Offset, count);

	m_Offset += count;

	Optimize();

	return count;
}

void *FIFO::GetWriteBuffer(size_t count)
{
	char *new_buffer;

	new_buffer = ResizeBuffer(m_Buffer, m_BufferSize, m_BufferSize + count);

	if (new_buffer == NULL)
		throw exception(/*"Out of memory."*/);

	m_Buffer = new_buffer;

	return new_buffer + m_Offset + m_BufferSize;
}

size_t FIFO::Write(const void *buffer, size_t count)
{
	if (buffer != NULL) {
		void *target_buffer = GetWriteBuffer(count);
		memcpy(target_buffer, buffer, count);
	}

	m_BufferSize += count;

	return count;
}
