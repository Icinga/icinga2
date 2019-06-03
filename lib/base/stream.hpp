/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STREAM_H
#define STREAM_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/signals2.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace icinga
{

class String;
class Stream;

enum ConnectionRole
{
	RoleClient,
	RoleServer
};

struct StreamReadContext
{
	~StreamReadContext()
	{
		free(Buffer);
	}

	bool FillFromStream(const intrusive_ptr<Stream>& stream, bool may_wait);
	void DropData(size_t count);

	char *Buffer{nullptr};
	size_t Size{0};
	bool MustRead{true};
	bool Eof{false};
};

enum StreamReadStatus
{
	StatusNewItem,
	StatusNeedData,
	StatusEof
};

/**
 * A stream.
 *
 * @ingroup base
 */
class Stream : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Stream);

	/**
	 * Reads data from the stream without removing it from the stream buffer.
	 *
	 * @param buffer The buffer where data should be stored. May be nullptr if you're
	 *		 not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 * @param allow_partial Whether to allow partial reads.
	 * @returns The number of bytes actually read.
	 */
	virtual size_t Peek(void *buffer, size_t count, bool allow_partial = false);

	/**
	 * Reads data from the stream.
	 *
	 * @param buffer The buffer where data should be stored. May be nullptr if you're
	 *		 not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 * @param allow_partial Whether to allow partial reads.
	 * @returns The number of bytes actually read.
	 */
	virtual size_t Read(void *buffer, size_t count, bool allow_partial = false) = 0;

	/**
	 * Writes data to the stream.
	 *
	 * @param buffer The data that is to be written.
	 * @param count The number of bytes to write.
	 * @returns The number of bytes written
	 */
	virtual void Write(const void *buffer, size_t count) = 0;

	/**
	 * Causes the stream to be closed (via Close()) once all pending data has been
	 * written.
	 */
	virtual void Shutdown();

	/**
	 * Closes the stream and releases resources.
	 */
	virtual void Close();

	/**
	 * Checks whether we've reached the end-of-file condition.
	 *
	 * @returns true if EOF.
	 */
	virtual bool IsEof() const = 0;

	/**
	 * Waits until data can be read from the stream.
	 * Optionally with a timeout.
	 */
	bool WaitForData();
	bool WaitForData(int timeout);

	virtual bool SupportsWaiting() const;

	virtual bool IsDataAvailable() const;

	void RegisterDataHandler(const std::function<void(const Stream::Ptr&)>& handler);

	StreamReadStatus ReadLine(String *line, StreamReadContext& context, bool may_wait = false);

protected:
	void SignalDataAvailable();

private:
	boost::signals2::signal<void(const Stream::Ptr&)> OnDataAvailable;

	boost::mutex m_Mutex;
	boost::condition_variable m_CV;
};

}

#endif /* STREAM_H */
