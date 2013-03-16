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

#ifndef FIFO_H
#define FIFO_H

#include "base/i2-base.h"
#include "base/stream.h"

namespace icinga
{

/**
 * A byte-based FIFO buffer.
 *
 * @ingroup base
 */
class I2_BASE_API FIFO : public Stream
{
public:
	static const size_t BlockSize = 16 * 1024;

	typedef shared_ptr<FIFO> Ptr;
	typedef weak_ptr<FIFO> WeakPtr;

	FIFO(void);
	~FIFO(void);

	void Start(void);

	size_t GetAvailableBytes(void) const;
	size_t Peek(void *buffer, size_t count);
	size_t Read(void *buffer, size_t count);
	void Write(const void *buffer, size_t count);

private:
	char *m_Buffer;
	size_t m_DataSize;
	size_t m_AllocSize;
	size_t m_Offset;

	void ResizeBuffer(size_t newSize);
	void Optimize(void);
};

}

#endif /* FIFO_H */
