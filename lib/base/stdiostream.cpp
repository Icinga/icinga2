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

#include "base/stdiostream.h"
#include "base/objectlock.h"
#include <boost/make_shared.hpp>

using namespace icinga;

/**
 * Constructor for the StdioStream class.
 *
 * @param innerStream The inner stream.
 * @param ownsStream Whether the new object owns the inner stream. If true
 *					 the stream's destructor deletes the inner stream.
 */
StdioStream::StdioStream(std::iostream *innerStream, bool ownsStream)
	: m_InnerStream(innerStream), m_OwnsStream(ownsStream),
	  m_ReadAheadBuffer(boost::make_shared<FIFO>())
{
	m_ReadAheadBuffer->Start();
}

/**
 * Destructor for the StdioStream class.
 */
StdioStream::~StdioStream(void)
{
	m_ReadAheadBuffer->Close();
}

/**
 * @threadsafety Always.
 */
void StdioStream::Start(void)
{
	SetConnected(true);

	Stream::Start();
}

/**
 * @threadsafety Always.
 */
size_t StdioStream::GetAvailableBytes(void) const
{
	ObjectLock olock(this);

	if (m_InnerStream->eof() && m_ReadAheadBuffer->GetAvailableBytes() == 0)
		return 0;
	else
		return 1024; /* doesn't have to be accurate */
}

/**
 * @threadsafety Always.
 */
size_t StdioStream::Read(void *buffer, size_t size)
{
	ObjectLock olock(this);

	size_t peek_len, read_len;

	peek_len = m_ReadAheadBuffer->GetAvailableBytes();
	peek_len = m_ReadAheadBuffer->Read(buffer, peek_len);

	m_InnerStream->read(static_cast<char *>(buffer) + peek_len, size - peek_len);
	read_len = m_InnerStream->gcount();

	return peek_len + read_len;
}

/**
 * @threadsafety Always.
 */
size_t StdioStream::Peek(void *buffer, size_t size)
{
	ObjectLock olock(this);

	size_t peek_len, read_len;

	peek_len = m_ReadAheadBuffer->GetAvailableBytes();
	peek_len = m_ReadAheadBuffer->Peek(buffer, peek_len);

	m_InnerStream->read(static_cast<char *>(buffer) + peek_len, size - peek_len);
	read_len = m_InnerStream->gcount();

	m_ReadAheadBuffer->Write(static_cast<char *>(buffer) + peek_len, read_len);
	return peek_len + read_len;
}

/**
 * @threadsafety Always.
 */
void StdioStream::Write(const void *buffer, size_t size)
{
	ObjectLock olock(this);

	m_InnerStream->write(static_cast<const char *>(buffer), size);
}

/**
 * @threadsafety Always.
 */
void StdioStream::Close(void)
{
	if (m_OwnsStream)
		delete m_InnerStream;

	Stream::Close();
}
