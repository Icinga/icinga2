/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef STREAM_H
#define STREAM_H

namespace icinga
{

/**
 * A stream.
 *
 * @ingroup base
 */
class I2_BASE_API Stream : public Object
{
public:
	typedef shared_ptr<Stream> Ptr;
	typedef weak_ptr<Stream> WeakPtr;

	Stream(void);
	~Stream(void);

	virtual void Start(void);

	/**
	 * Retrieves the number of bytes available for reading.
	 *
	 * @returns The number of available bytes.
	 */
	virtual size_t GetAvailableBytes(void) const = 0;

	/**
	 * Reads data from the stream without advancing the read pointer.
	 *
	 * @param buffer The buffer where data should be stored. May be NULL if
	 *		 you're not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 * @returns The number of bytes actually read.
	 */
	virtual size_t Peek(void *buffer, size_t count) = 0;

	/**
	 * Reads data from the stream.
	 *
	 * @param buffer The buffer where data should be stored. May be NULL if you're
	 *		 not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 * @returns The number of bytes actually read.
	 */
	virtual size_t Read(void *buffer, size_t count) = 0;

	/**
	 * Writes data to the stream.
	 *
	 * @param buffer The data that is to be written.
	 * @param count The number of bytes to write.
	 * @returns The number of bytes written
	 */
	virtual void Write(const void *buffer, size_t count) = 0;

	/**
	 * Closes the stream and releases resources.
	 */
	virtual void Close(void);

	bool IsConnected(void) const;
	bool IsReadEOF(void) const;
	bool IsWriteEOF(void) const;

	bool ReadLine(String *line, size_t maxLength = 4096);

	boost::exception_ptr GetException(void);
	void CheckException(void);

	signals2::signal<void (const Stream::Ptr&)> OnConnected;
	signals2::signal<void (const Stream::Ptr&)> OnDataAvailable;
	signals2::signal<void (const Stream::Ptr&)> OnClosed;

protected:
	void SetConnected(bool connected);
	void SetReadEOF(bool eof);
	void SetWriteEOF(bool eof);

	void SetException(boost::exception_ptr exception);

private:
	bool m_Running;
	bool m_Connected;
	bool m_ReadEOF;
	bool m_WriteEOF;
	boost::exception_ptr m_Exception;
};

BIO *BIO_Stream_new(const Stream::Ptr& stream);

}

#endif /* STREAM_H */
