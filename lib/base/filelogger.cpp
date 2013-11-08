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

#include "base/filelogger.h"
#include "base/dynamictype.h"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(FileLogger);

/**
 * Constructor for the FileLogger class.
 */
void FileLogger::Start()
{
	StreamLogger::Start();

	std::ofstream *stream = new std::ofstream();

	String path = GetPath();

	try {
		stream->open(path.CStr(), std::fstream::out | std::fstream::trunc);

		if (!stream->good())
			BOOST_THROW_EXCEPTION(std::runtime_error("Could not open logfile '" + path + "'"));
	} catch (...) {
		delete stream;
		throw;
	}

	BindStream(stream, true);
}
