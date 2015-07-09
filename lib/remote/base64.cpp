/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "remote/base64.hpp"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <sstream>

using namespace icinga;

const String base64_padding[] = {"", "==", "="};

String Base64::Encode(const String& data)
{
	typedef boost::archive::iterators::base64_from_binary <boost::archive::iterators::transform_width<const char *, 6, 8> > base64_encode;

	std::ostringstream msgbuf;
	std::copy(base64_encode(data.CStr()), base64_encode(data.CStr() + data.GetLength()), std::ostream_iterator<char>(msgbuf));
	msgbuf << base64_padding[data.GetLength() % 3];
	return msgbuf.str();
}

String Base64::Decode(const String& data)
{
	typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<const char *>, 8, 6> base64_decode;

	String::SizeType size = data.GetLength();

	if (size && data[size - 1] == '=') {
		--size;

		if (size && data[size - 1] == '=')
			--size;
	}

	if (size == 0)
		return String();

	std::ostringstream msgbuf;
	std::copy(base64_decode(data.CStr()), base64_decode(data.CStr() + size), std::ostream_iterator<char>(msgbuf));
	return msgbuf.str();
}
