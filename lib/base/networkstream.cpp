// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/networkstream.hpp"

using namespace icinga;

NetworkStream::NetworkStream(Socket::Ptr socket)
	: m_Socket(std::move(socket)), m_Eof(false)
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
size_t NetworkStream::Read(void *buffer, size_t count)
{
	size_t rc;

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
