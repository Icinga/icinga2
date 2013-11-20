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

#include "compat/externalcommandlistener.h"
#include "icinga/externalcommandprocessor.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/application.h"

using namespace icinga;

REGISTER_TYPE(ExternalCommandListener);

/**
 * Starts the component.
 */
void ExternalCommandListener::Start(void)
{
	DynamicObject::Start();

#ifndef _WIN32
	m_CommandThread = boost::thread(boost::bind(&ExternalCommandListener::CommandPipeThread, this, GetCommandPath()));
	m_CommandThread.detach();
#endif /* _WIN32 */
}

#ifndef _WIN32
void ExternalCommandListener::CommandPipeThread(const String& commandPath)
{
	Utility::SetThreadName("Command Pipe");

	struct stat statbuf;
	bool fifo_ok = false;

	if (lstat(commandPath.CStr(), &statbuf) >= 0) {
		if (S_ISFIFO(statbuf.st_mode) && access(commandPath.CStr(), R_OK) >= 0) {
			fifo_ok = true;
		} else {
			if (unlink(commandPath.CStr()) < 0) {
				BOOST_THROW_EXCEPTION(posix_error()
				    << boost::errinfo_api_function("unlink")
				    << boost::errinfo_errno(errno)
				    << boost::errinfo_file_name(commandPath));
			}
		}
	}

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

	if (!fifo_ok && mkfifo(commandPath.CStr(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("mkfifo")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(commandPath));
	}

	/* mkfifo() uses umask to mask off some bits, which means we need to chmod() the
	 * fifo to get the right mask. */
	if (chmod(commandPath.CStr(), mode) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("chmod")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(commandPath));
	}

	for (;;) {
		int fd;

		do {
			fd = open(commandPath.CStr(), O_RDONLY);
		} while (fd < 0 && errno == EINTR);

		if (fd < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("open")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(commandPath));
		}

		FILE *fp = fdopen(fd, "r");

		if (fp == NULL) {
			(void) close(fd);
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("fdopen")
			    << boost::errinfo_errno(errno));
		}

		char line[2048];

		while (fgets(line, sizeof(line), fp) != NULL) {
			// remove trailing new-line
			while (strlen(line) > 0 &&
			    (line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n'))
				line[strlen(line) - 1] = '\0';

			String command = line;

			try {
				Log(LogInformation, "compat", "Executing external command: " + command);

				ExternalCommandProcessor::Execute(command);
			} catch (const std::exception& ex) {
				std::ostringstream msgbuf;
				msgbuf << "External command failed: " << DiagnosticInformation(ex);
				Log(LogWarning, "compat", msgbuf.str());
			}
		}

		fclose(fp);
	}
}
#endif /* _WIN32 */
