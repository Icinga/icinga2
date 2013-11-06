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

#include "base/bufferedstream.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include <sstream>

using namespace icinga;

BufferedStream::BufferedStream(const Stream::Ptr& innerStream, size_t maxBufferSize)
	: m_InnerStream(innerStream), m_Stopped(false), m_Eof(false),
	  m_RecvQ(make_shared<FIFO>()), m_SendQ(make_shared<FIFO>()),
	  m_Blocking(true), m_MaxBufferSize(maxBufferSize), m_Exception()
{
	m_ReadThread = boost::thread(boost::bind(&BufferedStream::ReadThreadProc, this));
	m_WriteThread = boost::thread(boost::bind(&BufferedStream::WriteThreadProc, this));
}

BufferedStream::~BufferedStream(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		m_Stopped = true;
	}

	m_InnerStream->Close();

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		m_ReadCV.notify_all();
		m_WriteCV.notify_all();
	}

	m_ReadThread.join();
	m_WriteThread.join();
}

void BufferedStream::ReadThreadProc(void)
{
	char buffer[512];

	Utility::SetThreadName("BufS Read");

	try {
		for (;;) {
			size_t rc = m_InnerStream->Read(buffer, sizeof(buffer));

			if (rc == 0) {
				boost::mutex::scoped_lock lock(m_Mutex);
				m_Eof = true;
				m_Stopped = true;
				m_ReadCV.notify_all();
				m_WriteCV.notify_all();

				break;
			}

			boost::mutex::scoped_lock lock(m_Mutex);
			m_RecvQ->Write(buffer, rc);
			m_ReadCV.notify_all();

			if (m_Stopped)
				break;
		}
	} catch (const std::exception& ex) {
		{
			boost::mutex::scoped_lock lock(m_Mutex);

			if (!m_Exception)
				m_Exception = boost::current_exception();

			m_ReadCV.notify_all();
		}
	}
}

void BufferedStream::WriteThreadProc(void)
{
	char buffer[512];

	Utility::SetThreadName("BufS Write");

	try {
		for (;;) {
			size_t rc;

			{
				boost::mutex::scoped_lock lock(m_Mutex);

				while (m_SendQ->GetAvailableBytes() == 0 && !m_Stopped)
					m_WriteCV.wait(lock);

				if (m_Stopped)
					break;

				rc = m_SendQ->Read(buffer, sizeof(buffer));
				m_WriteCV.notify_all();
			}

			m_InnerStream->Write(buffer, rc);
		}
	} catch (const std::exception& ex) {
		{
			boost::mutex::scoped_lock lock(m_Mutex);

			if (!m_Exception)
				m_Exception = boost::current_exception();

			m_WriteCV.notify_all();
		}
	}
}

void BufferedStream::Close(void)
{
	m_InnerStream->Close();
}

/**
 * Reads data from the stream.
 *
 * @param buffer The buffer where data should be stored. May be NULL if you're
 *		 not actually interested in the data.
 * @param count The number of bytes to read from the queue.
 * @returns The number of bytes actually read.
 */
size_t BufferedStream::Read(void *buffer, size_t count)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	if (m_Blocking)
		InternalWaitReadable(count, lock);

	if (m_Exception)
		boost::rethrow_exception(m_Exception);

	if (m_Eof)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to read from closed socket."));

	return m_RecvQ->Read(buffer, count);
}

/**
 * Writes data to the stream.
 *
 * @param buffer The data that is to be written.
 * @param count The number of bytes to write.
 */
void BufferedStream::Write(const void *buffer, size_t count)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	InternalWaitWritable(count, lock);

	if (m_Exception)
		boost::rethrow_exception(m_Exception);

	if (m_Eof)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to write to closed socket."));

	m_SendQ->Write(buffer, count);
	m_WriteCV.notify_all();
}

void BufferedStream::WaitReadable(size_t count)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	InternalWaitReadable(count, lock);
}

void BufferedStream::InternalWaitReadable(size_t count, boost::mutex::scoped_lock& lock)
{
	while (m_RecvQ->GetAvailableBytes() < count && !m_Exception && !m_Stopped)
		m_ReadCV.wait(lock);
}

void BufferedStream::WaitWritable(size_t count)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	InternalWaitWritable(count, lock);
}

void BufferedStream::InternalWaitWritable(size_t count, boost::mutex::scoped_lock& lock)
{
	while (m_SendQ->GetAvailableBytes() + count > m_MaxBufferSize && !m_Exception && !m_Stopped)
		m_WriteCV.wait(lock);
}

void BufferedStream::MakeNonBlocking(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	m_Blocking = false;
}

bool BufferedStream::IsEof(void) const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_InnerStream->IsEof() && m_RecvQ->GetAvailableBytes() == 0;
}
