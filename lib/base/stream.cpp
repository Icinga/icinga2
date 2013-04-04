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

#include "base/stream.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include <boost/algorithm/string/trim.hpp>

using namespace icinga;

bool Stream::ReadLine(String *line, size_t maxLength)
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented."));
	/*
	char *buffer = new char[maxLength];

	size_t rc = Peek(buffer, maxLength);

	if (rc == 0)
		return false;

	for (size_t i = 0; i < rc; i++) {
		if (buffer[i] == '\n') {
			*line = String(buffer, &(buffer[i]));
			boost::algorithm::trim_right(*line);

			Read(NULL, i + 1);

			delete buffer;

			return true;
		}
	}

	if (IsReadEOF()) {
		*line = String(buffer, buffer + rc);
		boost::algorithm::trim_right(*line);

		Read(NULL, rc);

		delete buffer;

		return true;
	}

	delete buffer;*/

	return false;
}
