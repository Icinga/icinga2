/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/stdiostream.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

/**
 * Constructor for the StdioStream class.
 *
 * @param innerStream The inner stream.
 * @param ownsStream Whether the new object owns the inner stream. If true
 *					 the stream's destructor deletes the inner stream.
 */
StdioStream::StdioStream(std::iostream *innerStream, bool ownsStream)
	: m_InnerStream(innerStream), m_OwnsStream(ownsStream)
{ }

StdioStream::~StdioStream()
{
	Close();
}

size_t StdioStream::Read(void *buffer, size_t size, bool allow_partial)
{
	ObjectLock olock(this);

	m_InnerStream->read(static_cast<char *>(buffer), size);
	return m_InnerStream->gcount();
}

void StdioStream::Write(const void *buffer, size_t size)
{
	ObjectLock olock(this);

	m_InnerStream->write(static_cast<const char *>(buffer), size);
}

void StdioStream::Close()
{
	Stream::Close();

	if (m_OwnsStream) {
		delete m_InnerStream;
		m_OwnsStream = false;
	}
}

bool StdioStream::IsDataAvailable() const
{
	return !IsEof();
}

bool StdioStream::IsEof() const
{
	return !m_InnerStream->good();
}
