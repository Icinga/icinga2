/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/httpchunkedencoding.hpp"
#include <sstream>

using namespace icinga;

StreamReadStatus HttpChunkedEncoding::ReadChunkFromStream(const Stream::Ptr& stream,
	char **data, size_t *size, ChunkReadContext& context, bool may_wait)
{
	if (context.LengthIndicator == -1) {
		String line;
		StreamReadStatus status = stream->ReadLine(&line, context.StreamContext, may_wait);
		may_wait = false;

		if (status != StatusNewItem)
			return status;

		std::stringstream msgbuf;
		msgbuf << std::hex << line;
		msgbuf >> context.LengthIndicator;

		if (context.LengthIndicator < 0)
			BOOST_THROW_EXCEPTION(std::invalid_argument("HTTP chunk length must not be negative."));
	}

	StreamReadContext& scontext = context.StreamContext;
	if (scontext.Eof)
		return StatusEof;

	if (scontext.MustRead) {
		if (!scontext.FillFromStream(stream, may_wait)) {
			scontext.Eof = true;
			return StatusEof;
		}

		scontext.MustRead = false;
	}

	size_t NewlineLength = context.LengthIndicator ? 2 : 0;

	if (scontext.Size < (size_t)context.LengthIndicator + NewlineLength) {
		scontext.MustRead = true;
		return StatusNeedData;
	}

	*data = new char[context.LengthIndicator];
	*size = context.LengthIndicator;
	memcpy(*data, scontext.Buffer, context.LengthIndicator);

	scontext.DropData(context.LengthIndicator + NewlineLength);
	context.LengthIndicator = -1;

	return StatusNewItem;
}

void HttpChunkedEncoding::WriteChunkToStream(const Stream::Ptr& stream, const char *data, size_t count)
{
	std::ostringstream msgbuf;
	msgbuf << std::hex << count << "\r\n";
	String lengthIndicator = msgbuf.str();
	stream->Write(lengthIndicator.CStr(), lengthIndicator.GetLength());
	stream->Write(data, count);
	if (count > 0)
		stream->Write("\r\n", 2);
}
