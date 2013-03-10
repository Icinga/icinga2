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

Stream::Stream(void)
	: m_Connected(false), m_ReadEOF(false), m_WriteEOF(false)
{ }

Stream::~Stream(void)
{
	ASSERT(!m_Running);
}

/**
 * @threadsafety Always.
 */
bool Stream::IsConnected(void) const
{
	ObjectLock olock(this);

	return m_Connected;
}

/**
 * @threadsafety Always.
 */
bool Stream::IsReadEOF(void) const
{
	ObjectLock olock(this);

	return m_ReadEOF;
}

/**
 * @threadsafety Always.
 */
bool Stream::IsWriteEOF(void) const
{
	ObjectLock olock(this);

	return m_WriteEOF;
}

/**
 * @threadsafety Always.
 */
void Stream::SetConnected(bool connected)
{
	bool changed;

	{
		ObjectLock olock(this);
		changed = (m_Connected != connected);
		m_Connected = connected;
	}

	if (changed) {
		if (connected)
			OnConnected(GetSelf());
		else
			OnClosed(GetSelf());
	}
}

/**
 * @threadsafety Always.
 */
void Stream::SetReadEOF(bool eof)
{
	ObjectLock olock(this);

	m_ReadEOF = eof;
}

/**
 * @threadsafety Always.
 */
void Stream::SetWriteEOF(bool eof)
{
	ObjectLock olock(this);

	m_WriteEOF = eof;
}

/**
 * Checks whether an exception is available for this stream and re-throws
 * the exception if there is one.
 *
 * @threadsafety Always.
 */
void Stream::CheckException(void)
{
	ObjectLock olock(this);

	if (m_Exception)
		rethrow_exception(m_Exception);
}

/**
 * @threadsafety Always.
 */
void Stream::SetException(boost::exception_ptr exception)
{
	ObjectLock olock(this);

	m_Exception = exception;
}

/**
 * @threadsafety Always.
 */
boost::exception_ptr Stream::GetException(void)
{
	return m_Exception;
}

/**
 * @threadsafety Always.
 */
void Stream::Start(void)
{
	ObjectLock olock(this);

	m_Running = true;
}

/**
 * @threadsafety Always.
 */
void Stream::Close(void)
{
	{
		ObjectLock olock(this);

		m_Running = false;
	}

	SetConnected(false);
}

bool Stream::ReadLine(String *line, size_t maxLength)
{
	char *buffer = new char[maxLength];

	size_t rc = Peek(buffer, maxLength);

	if (rc == 0)
		return false;

	for (size_t i = 0; i < rc; i++) {
		if (buffer[i] == '\n') {
			*line = String(buffer, &(buffer[i]));
			boost::algorithm::trim_right(*line);

			Read(NULL, i + 1);

			delete buffer;

			return true;
		}
	}

	if (IsReadEOF()) {
		*line = String(buffer, buffer + rc);
		boost::algorithm::trim_right(*line);

		Read(NULL, rc);

		delete buffer;

		return true;
	}

	delete buffer;

	return false;
}
