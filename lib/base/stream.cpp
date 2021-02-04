/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/stream.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <chrono>

using namespace icinga;

void Stream::RegisterDataHandler(const std::function<void(const Stream::Ptr&)>& handler)
{
	if (SupportsWaiting())
		OnDataAvailable.connect(handler);
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support waiting."));
}

bool Stream::SupportsWaiting() const
{
	return false;
}

bool Stream::IsDataAvailable() const
{
	return false;
}

void Stream::Shutdown()
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support Shutdown()."));
}

size_t Stream::Peek(void *buffer, size_t count, bool allow_partial)
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support Peek()."));
}

void Stream::SignalDataAvailable()
{
	OnDataAvailable(this);

	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_CV.notify_all();
	}
}

bool Stream::WaitForData()
{
	if (!SupportsWaiting())
		BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support waiting."));

	std::unique_lock<std::mutex> lock(m_Mutex);

	while (!IsDataAvailable() && !IsEof())
		m_CV.wait(lock);

	return IsDataAvailable() || IsEof();
}

bool Stream::WaitForData(int timeout)
{
	namespace ch = std::chrono;

	if (!SupportsWaiting())
		BOOST_THROW_EXCEPTION(std::runtime_error("Stream does not support waiting."));

	if (timeout < 0)
		BOOST_THROW_EXCEPTION(std::runtime_error("Timeout can't be negative"));

	std::unique_lock<std::mutex> lock(m_Mutex);

	return m_CV.wait_for(lock, ch::duration<int>(timeout), [this]() { return IsDataAvailable() || IsEof(); });
}

static void StreamDummyCallback()
{ }

void Stream::Close()
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

	for (size_t i = 0; i < context.Size; i++) {
		if (context.Buffer[i] == '\n') {
			*line = String(context.Buffer, context.Buffer + i);
			boost::algorithm::trim_right(*line);

			context.DropData(i + 1u);

			context.MustRead = !context.Size;
			return StatusNewItem;
		}
	}

	context.MustRead = true;
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

		if (stream->IsEof())
			break;

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
