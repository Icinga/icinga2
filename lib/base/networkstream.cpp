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

#include "base/networkstream.hpp"

using namespace icinga;

NetworkStream::NetworkStream(const Socket::Ptr& socket)
	: m_Socket(socket), m_Eof(false)
{ }

void NetworkStream::Close()
{
	Stream::Close();

	m_Socket->Close();
}

/**
 * Reads data from the stream.
 *
 * @param buffer The buffer where data should be stored. May be nullptr if you're
 *		 not actually interested in the data.
 * @param count The number of bytes to read from the queue.
 * @returns The number of bytes actually read.
 */
size_t NetworkStream::Read(void *buffer, size_t count, bool allow_partial)
{
	size_t rc;

	ASSERT(allow_partial);

	if (m_Eof)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to read from closed socket."));

	try {
		rc = m_Socket->Read(buffer, count);
	} catch (...) {
		m_Eof = true;

		throw;
	}

	if (rc == 0)
		m_Eof = true;

	return rc;
}

/**
 * Writes data to the stream.
 *
 * @param buffer The data that is to be written.
 * @param count The number of bytes to write.
 * @returns The number of bytes written
 */
void NetworkStream::Write(const void *buffer, size_t count)
{
	size_t rc;

	if (m_Eof)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to write to closed socket."));

	try {
		rc = m_Socket->Write(buffer, count);
	} catch (...) {
		m_Eof = true;

		throw;
	}

	if (rc < count) {
		m_Eof = true;

		BOOST_THROW_EXCEPTION(std::runtime_error("Short write for socket."));
	}
}

bool NetworkStream::IsEof() const
{
	return m_Eof;
}
