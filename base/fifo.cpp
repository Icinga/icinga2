/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the FIFO class.
 */
FIFO::FIFO(void)
{
	m_Buffer = NULL;
	m_DataSize = 0;
	m_AllocSize = 0;
	m_Offset = 0;
}

/**
 * Destructor for the FIFO class.
 */
FIFO::~FIFO(void)
{
	free(m_Buffer);
}

/**
 * Resizes the FIFO's buffer so that it is at least newSize bytes long.
 *
 * @param newSize The minimum new size of the FIFO buffer.
 */
void FIFO::ResizeBuffer(size_t newSize)
{
	if (m_AllocSize >= newSize)
		return;

	newSize = (newSize / FIFO::BlockSize + 1) * FIFO::BlockSize;

	char *newBuffer = (char *)realloc(m_Buffer, newSize);

	if (newBuffer == NULL)
		throw bad_alloc();

	m_Buffer = newBuffer;

	m_AllocSize = newSize;
}

/**
 * Optimizes memory usage of the FIFO buffer by reallocating
 * and moving the buffer.
 */
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

/**
 * Returns the number of bytes that are contained in the FIFO.
 *
 * @returns The number of bytes.
 */
size_t FIFO::GetSize(void) const
{
	return m_DataSize;
}

/**
 * Returns a pointer to the start of the read buffer.
 *
 * @returns Pointer to the read buffer.
 */
const void *FIFO::GetReadBuffer(void) const
{
	return m_Buffer + m_Offset;
}

/**
 * Reads data from the FIFO and places it in the specified buffer.
 *
 * @param buffer The buffer where the data should be placed (can be NULL if
 *               the reader is not interested in the data).
 * @param count The number of bytes to read.
 * @returns The number of bytes read which may be less than what was requested.
 */
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

/**
 * Returns a pointer to the start of the write buffer.
 *
 * @param count Minimum size of the buffer; on return this parameter
 *              contains the actual size of the available buffer which can
 *              be larger than the requested size.
 */
void *FIFO::GetWriteBuffer(size_t *count)
{
	ResizeBuffer(m_Offset + m_DataSize + *count);
	*count = m_AllocSize - m_Offset - m_DataSize;

	return m_Buffer + m_Offset + m_DataSize;
}

/**
 * Writes data to the FIFO.
 *
 * @param buffer The data that is to be written (can be NULL if the writer has
 *               already filled the write buffer, e.g. via GetWriteBuffer()).
 * @param count The number of bytes to write.
 * @returns The number of bytes written
 */
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
