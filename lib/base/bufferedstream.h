/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef BUFFEREDSTREAM_H
#define BUFFEREDSTREAM_H

#include "base/i2-base.h"
#include "base/stream.h"
#include "base/fifo.h"

namespace icinga
{

/**
 * A buffered stream.
 *
 * @ingroup base
 */
class I2_BASE_API BufferedStream : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(BufferedStream);

	BufferedStream(const Stream::Ptr& innerStream, size_t maxBufferSize = 64 * 1024 * 1024);
	~BufferedStream(void);

	virtual size_t Read(void *buffer, size_t count);
	virtual void Write(const void *buffer, size_t count);

	virtual void Close(void);

	virtual bool IsEof(void) const;

	void WaitReadable(size_t count);
	void WaitWritable(size_t count);

	void MakeNonBlocking(void);

private:
	Stream::Ptr m_InnerStream;

	bool m_Stopped;
	bool m_Eof;

	FIFO::Ptr m_RecvQ;
	FIFO::Ptr m_SendQ;

	bool m_Blocking;
	size_t m_MaxBufferSize;

	boost::exception_ptr m_Exception;

	mutable boost::mutex m_Mutex;
	boost::condition_variable m_ReadCV;
	boost::condition_variable m_WriteCV;

	void ReadThreadProc(void);
	void WriteThreadProc(void);

	boost::thread m_ReadThread;
	boost::thread m_WriteThread;

	void InternalWaitWritable(size_t count, boost::mutex::scoped_lock& lock);
	void InternalWaitReadable(size_t count, boost::mutex::scoped_lock& lock);
};

}

#endif /* BUFFEREDSTREAM_H */
