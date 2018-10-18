/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef HTTPCHUNKEDENCODING_H
#define HTTPCHUNKEDENCODING_H

#include "remote/i2-remote.hpp"
#include "base/stream.hpp"

namespace icinga
{

struct ChunkReadContext
{
	StreamReadContext& StreamContext;
	int LengthIndicator;

	ChunkReadContext(StreamReadContext& scontext)
		: StreamContext(scontext), LengthIndicator(-1)
	{ }
};

/**
 * HTTP chunked encoding.
 *
 * @ingroup remote
 */
struct HttpChunkedEncoding
{
	static StreamReadStatus ReadChunkFromStream(const Stream::Ptr& stream,
		char **data, size_t *size, ChunkReadContext& ccontext, bool may_wait = false);
	static void WriteChunkToStream(const Stream::Ptr& stream, const char *data, size_t count);

};

}

#endif /* HTTPCHUNKEDENCODING_H */
