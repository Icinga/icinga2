/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
