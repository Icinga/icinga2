// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BASE64_H
#define BASE64_H

#include "remote/i2-remote.hpp"
#include "base/string.hpp"

namespace icinga
{

/**
 * Base64
 *
 * @ingroup remote
 */
struct Base64
{
	static String Decode(const String& data);
	static String Encode(const String& data);
};

}

#endif /* BASE64_H */
