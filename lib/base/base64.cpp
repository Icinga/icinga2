// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
