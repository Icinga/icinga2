// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

size_t StdioStream::Read(void *buffer, size_t size)
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
