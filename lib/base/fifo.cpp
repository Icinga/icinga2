/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/fifo.hpp"

using namespace icinga;

/**
 * Destructor for the FIFO class.
 */
FIFO::~FIFO()
{
	free(m_Buffer);
}

/**
 * Resizes the FIFO's buffer so that it is at least newSize bytes long.
 *
 * @param newSize The minimum new size of the FIFO buffer.
 */
void FIFO::ResizeBuffer(size_t newSize, bool decrease)
{
	if (m_AllocSize >= newSize && !decrease)
		return;

	newSize = (newSize / FIFO::BlockSize + 1) * FIFO::BlockSize;

	if (newSize == m_AllocSize)
		return;

	auto *newBuffer = static_cast<char *>(realloc(m_Buffer, newSize));

	if (!newBuffer)
		BOOST_THROW_EXCEPTION(std::bad_alloc());

	m_Buffer = newBuffer;

	m_AllocSize = newSize;
}

/**
 * Optimizes memory usage of the FIFO buffer by reallocating
 * and moving the buffer.
 */
void FIFO::Optimize()
{
	if (m_Offset > m_DataSize / 10 && m_Offset - m_DataSize > 1024) {
		std::memmove(m_Buffer, m_Buffer + m_Offset, m_DataSize);
		m_Offset = 0;

		if (m_DataSize > 0)
			ResizeBuffer(m_DataSize, true);

		return;
	}
}

/**
 * Implements IOQueue::Read.
 */
size_t FIFO::Read(void *buffer, size_t count, bool allow_partial)
{
	ASSERT(allow_partial);

	if (count > m_DataSize)
		count = m_DataSize;

	if (buffer)
		std::memcpy(buffer, m_Buffer + m_Offset, count);

	m_DataSize -= count;
	m_Offset += count;

	Optimize();

	return count;
}

/**
 * Implements IOQueue::Write.
 */
void FIFO::Write(const void *buffer, size_t count)
{
	ResizeBuffer(m_Offset + m_DataSize + count, false);
	std::memcpy(m_Buffer + m_Offset + m_DataSize, buffer, count);
	m_DataSize += count;

	SignalDataAvailable();
}

void FIFO::Close()
{ }

bool FIFO::IsEof() const
{
	return false;
}

size_t FIFO::GetAvailableBytes() const
{
	return m_DataSize;
}

bool FIFO::SupportsWaiting() const
{
	return true;
}

bool FIFO::IsDataAvailable() const
{
	return m_DataSize > 0;
}
