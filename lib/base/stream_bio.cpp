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

#include "base/stream_bio.h"

using namespace icinga;

static int I2Stream_new(BIO *bi);
static int I2Stream_free(BIO *bi);
static int I2Stream_read(BIO *bi, char *out, int outl);
static int I2Stream_write(BIO *bi, const char *in, int inl);
static long I2Stream_ctrl(BIO *bi, int cmd, long num, void *ptr);

#define BIO_TYPE_I2STREAM		(99|0x0400|0x0100)

static BIO_METHOD I2Stream_method =
{
	BIO_TYPE_I2STREAM,
	"Icinga Stream",
	I2Stream_write,
	I2Stream_read,
	NULL,
	NULL,
	I2Stream_ctrl,
	I2Stream_new,
	I2Stream_free,
	NULL,
};

typedef struct I2Stream_bio_s
{
	Stream::Ptr StreamObj;
	boost::exception_ptr Exception;
} I2Stream_bio_t;

BIO_METHOD *BIO_s_I2Stream(void)
{
	return &I2Stream_method;
}

BIO *icinga::BIO_new_I2Stream(const Stream::Ptr& stream)
{
	BIO *bi = BIO_new(BIO_s_I2Stream());

	if (bi == NULL)
		return NULL;

	I2Stream_bio_t *bp = (I2Stream_bio_t *)bi->ptr;

	bp->StreamObj = stream;

	return bi;
}

void icinga::I2Stream_check_exception(BIO *bi) {
	I2Stream_bio_t *bp = (I2Stream_bio_t *)bi->ptr;

	if (bp->Exception) {
		boost::exception_ptr ptr = bp->Exception;
		bp->Exception = boost::exception_ptr();
		rethrow_exception(ptr);
	}
}

static int I2Stream_new(BIO *bi)
{
	bi->shutdown = 0;
	bi->init = 1;
	bi->num = -1;
	bi->ptr = new I2Stream_bio_t;

	return 1;
}

static int I2Stream_free(BIO *bi)
{
	I2Stream_bio_t *bp = (I2Stream_bio_t *)bi->ptr;
	delete bp;

	return 1;
}

static int I2Stream_read(BIO *bi, char *out, int outl)
{
	I2Stream_bio_t *bp = (I2Stream_bio_t *)bi->ptr;

	size_t data_read;

	BIO_clear_retry_flags(bi);

	try {
		data_read = bp->StreamObj->Read(out, outl);
	} catch (...) {
		bp->Exception = boost::current_exception();
		return -1;
	}

	if (data_read == 0 && !bp->StreamObj->IsEof()) {
		BIO_set_retry_read(bi);
		return -1;
	}

	return data_read;
}

static int I2Stream_write(BIO *bi, const char *in, int inl)
{
	I2Stream_bio_t *bp = (I2Stream_bio_t *)bi->ptr;
	bp->StreamObj->Write(in, inl);
	return inl;
}

static long I2Stream_ctrl(BIO *, int cmd, long, void *)
{
	switch (cmd) {
		case BIO_CTRL_FLUSH:
			return 1;
		default:
			return 0;
	}
}
