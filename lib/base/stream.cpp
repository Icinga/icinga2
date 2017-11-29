/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/stream.hpp"
#include <boost/algorithm/string/trim.hpp>

using namespace icinga;

void Stream::RegisterDataHandler(const std::function<void(const Stream::Ptr&)>& handler)
{
	if (SupportsWaiting())
		OnDataAvailable.connect(handler);
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support waiting."));
}

bool Stream::SupportsWaiting(void) const
{
	return false;
}

bool Stream::IsDataAvailable(void) const
{
	return false;
}

void Stream::Shutdown(void)
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support Shutdown()."));
}

size_t Stream::Peek(void *buffer, size_t count, bool allow_partial)
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support Peek()."));
}

void Stream::SignalDataAvailable(void)
{
	OnDataAvailable(this);

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_CV.notify_all();
	}
}

bool Stream::WaitForData(int timeout)
{
	if (!SupportsWaiting())
		BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support waiting."));

	boost::mutex::scoped_lock lock(m_Mutex);

	while (!IsDataAvailable() && !IsEof())
		if (timeout < 0)
			m_CV.wait(lock);
		else
			m_CV.timed_wait(lock, boost::posix_time::milliseconds(timeout * 1000));

	return IsDataAvailable() || IsEof();
}

static void StreamDummyCallback(void)
{ }

void Stream::Close(void)
{
	OnDataAvailable.disconnect_all_slots();

	/* Force signals2 to remove the slots, see https://stackoverflow.com/questions/2049291/force-deletion-of-slot-in-boostsignals2
	 * for details. */
	OnDataAvailable.connect(std::bind(&StreamDummyCallback));
}

StreamReadStatus Stream::ReadLine(String *line, StreamReadContext& context, bool may_wait)
{
	if (context.Eof)
		return StatusEof;

	if (context.MustRead) {
		if (!context.FillFromStream(this, may_wait)) {
			context.Eof = true;

			*line = String(context.Buffer, &(context.Buffer[context.Size]));
			boost::algorithm::trim_right(*line);

			return StatusNewItem;
		}
	}

	int count = 0;
	size_t first_newline;

	for (size_t i = 0; i < context.Size; i++) {
		if (context.Buffer[i] == '\n') {
			count++;

			if (count == 1)
				first_newline = i;
			else if (count > 1)
				break;
		}
	}

	context.MustRead = (count <= 1);

	if (count > 0) {
		*line = String(context.Buffer, &(context.Buffer[first_newline]));
		boost::algorithm::trim_right(*line);

		context.DropData(first_newline + 1);

		return StatusNewItem;
	}

	return StatusNeedData;
}

bool StreamReadContext::FillFromStream(const Stream::Ptr& stream, bool may_wait)
{
	if (may_wait && stream->SupportsWaiting())
		stream->WaitForData();

	size_t count = 0;

	do {
		Buffer = (char *)realloc(Buffer, Size + 4096);

		if (!Buffer)
			throw std::bad_alloc();

		size_t rc = stream->Read(Buffer + Size, 4096, true);

		Size += rc;
		count += rc;
	} while (count < 64 * 1024 && stream->IsDataAvailable());

	if (count == 0 && stream->IsEof())
		return false;
	else
		return true;
}

void StreamReadContext::DropData(size_t count)
{
	ASSERT(count <= Size);
	memmove(Buffer, Buffer + count, Size - count);
	Size -= count;
}
