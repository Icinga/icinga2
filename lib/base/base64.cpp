/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/base64.hpp"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <sstream>

using namespace icinga;

String Base64::Encode(const String& input)
{
	BIO *biomem = BIO_new(BIO_s_mem());
	BIO *bio64 = BIO_new(BIO_f_base64());
	BIO_push(bio64, biomem);
	BIO_set_flags(bio64, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio64, input.CStr(), input.GetLength());
	(void) BIO_flush(bio64);

	char *outbuf;
	long len = BIO_get_mem_data(biomem, &outbuf);

	String ret = String(outbuf, outbuf + len);
	BIO_free_all(bio64);

	return ret;
}

String Base64::Decode(const String& input)
{
	BIO *biomem = BIO_new_mem_buf(
		const_cast<char*>(input.CStr()), input.GetLength());
	BIO *bio64 = BIO_new(BIO_f_base64());
	BIO_push(bio64, biomem);
	BIO_set_flags(bio64, BIO_FLAGS_BASE64_NO_NL);

	auto *outbuf = new char[input.GetLength()];

	size_t len = 0;
	int rc;

	while ((rc = BIO_read(bio64, outbuf + len, input.GetLength() - len)) > 0)
		len += rc;

	String ret = String(outbuf, outbuf + len);
	BIO_free_all(bio64);
	delete [] outbuf;

	if (ret.IsEmpty() && !input.IsEmpty())
		throw std::invalid_argument("Not a valid base64 string");

	return ret;
}
