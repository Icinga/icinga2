#ifndef IOQUEUE_H
#define IOQUEUE_H

namespace icinga
{

/**
 * An I/O queue.
 */
class IOQueue
{
public:
	/**
	 * Retrieves the number of bytes available for reading.
	 *
	 * @returns The number of available bytes.
	 */
	virtual size_t GetAvailableBytes(void) const = 0;

	/**
	 * Reads data from the queue without advancing the read pointer. Trying
	 * to read more data than is available in the queue is a programming error.
	 * Use GetBytesAvailable() to check how much data is available.
	 *
	 * @buffer The buffer where data should be stored. May be NULL if you're
	 *		 not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 */
	virtual void Peek(void *buffer, size_t count) = 0;

	/**
	 * Reads data from the queue. Trying to read more data than is
	 * available in the queue is a programming error. Use GetBytesAvailable()
	 * to check how much data is available.
	 *
	 * @param buffer The buffer where data should be stored. May be NULL if you're
	 *		 not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 */
	virtual void Read(void *buffer, size_t count) = 0;

	/**
	 * Writes data to the queue.
	 *
	 * @param buffer The data that is to be written.
	 * @param count The number of bytes to write.
	 * @returns The number of bytes written
	 */
	virtual void Write(const void *buffer, size_t count) = 0;
};

}

#endif /* IOQUEUE_H */