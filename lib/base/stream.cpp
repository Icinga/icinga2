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

#include "base/stream.hpp"
#include <boost/algorithm/string/trim.hpp>

using namespace icinga;

bool Stream::ReadLine(String *line, ReadLineContext& context)
{
	if (context.Eof)
		return false;

	for (;;) {
		if (context.MustRead) {
			context.Buffer = (char *)realloc(context.Buffer, context.Size + 4096);

			if (!context.Buffer)
				throw std::bad_alloc();

			size_t rc = Read(context.Buffer + context.Size, 4096);

			if (rc == 0) {
				*line = String(context.Buffer, &(context.Buffer[context.Size]));
				boost::algorithm::trim_right(*line);

				context.Eof = true;

				return true;
			}

			context.Size += rc;
		}

		int count = 0;
		size_t first_newline;

		for (size_t i = 0; i < context.Size; i++) {
			if (context.Buffer[i] == '\n') {
				count++;

				if (count == 1)
					first_newline = i;
			}
		}

		context.MustRead = (count <= 1);

		if (count > 0) {
			*line = String(context.Buffer, &(context.Buffer[first_newline]));
			boost::algorithm::trim_right(*line);

			memmove(context.Buffer, context.Buffer + first_newline + 1, context.Size - first_newline - 1);
			context.Size -= first_newline + 1;

			return true;
		}
	}
}
