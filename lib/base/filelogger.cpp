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

#include "base/filelogger.hpp"
#include "base/filelogger.tcpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include "base/application.hpp"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(FileLogger);

REGISTER_STATSFUNCTION(FileLogger, &FileLogger::StatsFunc);

void FileLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const FileLogger::Ptr& filelogger : ConfigType::GetObjectsByType<FileLogger>()) {
		nodes->Set(filelogger->GetName(), 1); //add more stats
	}

	status->Set("filelogger", nodes);
}

/**
 * Constructor for the FileLogger class.
 */
void FileLogger::Start(bool runtimeCreated)
{
	ReopenLogFile();

	Application::OnReopenLogs.connect(std::bind(&FileLogger::ReopenLogFile, this));

	ObjectImpl<FileLogger>::Start(runtimeCreated);
}

void FileLogger::ReopenLogFile()
{
	std::ofstream *stream = new std::ofstream();

	String path = GetPath();

	try {
		stream->open(path.CStr(), std::fstream::app | std::fstream::out);

		if (!stream->good())
			BOOST_THROW_EXCEPTION(std::runtime_error("Could not open logfile '" + path + "'"));
	} catch (...) {
		delete stream;
		throw;
	}

	BindStream(stream, true);
}
