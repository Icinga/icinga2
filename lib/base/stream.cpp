/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

void Stream::RegisterDataHandler(const boost::function<void(void)>& handler)
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

void Stream::SignalDataAvailable(void)
{
	OnDataAvailable();

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_CV.notify_all();
	}
}

void Stream::WaitForData(void)
{
	if (!SupportsWaiting())
		BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support waiting."));

	boost::mutex::scoped_lock lock(m_Mutex);

	while (!IsDataAvailable())
		m_CV.wait(lock);
}

StreamReadStatus Stream::ReadLine(String *line, StreamReadContext& context)
{
	if (IsEof())
		return StatusEof;

	if (context.MustRead) {
		if (!context.FillFromStream(this)) {
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

bool StreamReadContext::FillFromStream(const Stream::Ptr& stream)
{
	if (Wait && stream->SupportsWaiting())
		stream->WaitForData();

	do {
		Buffer = (char *)realloc(Buffer, Size + 4096);

		if (!Buffer)
			throw std::bad_alloc();

		size_t rc = stream->Read(Buffer + Size, 4096, true);

		if (rc == 0 && stream->IsEof())
			return false;

		Size += rc;
	} while (stream->IsDataAvailable());

	return true;
}

void StreamReadContext::DropData(size_t count)
{
	memmove(Buffer, Buffer + count, Size - count);
	Size -= count;
}
