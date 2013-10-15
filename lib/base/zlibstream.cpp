/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/zlibstream.h"
#include "base/objectlock.h"
#include <boost/make_shared.hpp>

#ifdef HAVE_BIOZLIB

using namespace icinga;

extern "C" BIO_METHOD *BIO_f_zlib(void);

/**
 * Constructor for the ZlibStream class.
 *
 * @param innerStream The inner stream.
 * @param compress Whether we're compressing, false if we're decompressing.
 */
ZlibStream::ZlibStream(const Stream::Ptr& innerStream)
	: m_InnerStream(innerStream)
{
	BIO *ibio = BIO_new_I2Stream(innerStream);
	BIO *zbio = BIO_new(BIO_f_zlib());
	m_BIO = BIO_push(zbio, ibio);
}

ZlibStream::~ZlibStream(void)
{
	Close();
}

size_t ZlibStream::Read(void *buffer, size_t size)
{
	ObjectLock olock(this);

	return BIO_read(m_BIO, buffer, size);
}

void ZlibStream::Write(const void *buffer, size_t size)
{
	ObjectLock olock(this);

	BIO_write(m_BIO, buffer, size);
}

void ZlibStream::Close(void)
{
	ObjectLock olock(this);

	if (m_BIO) {
		BIO_free_all(m_BIO);
		m_BIO = NULL;

		m_InnerStream->Close();
	}
}

bool ZlibStream::IsEof(void) const
{
	ObjectLock olock(this);

	return BIO_eof(m_BIO);
}

#endif /* HAVE_BIOZLIB */
