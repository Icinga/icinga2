/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "compat/externalcommandlistener.hpp"
#include "compat/externalcommandlistener.tcpp"
#include "icinga/externalcommandprocessor.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

REGISTER_TYPE(ExternalCommandListener);

REGISTER_STATSFUNCTION(ExternalCommandListener, &ExternalCommandListener::StatsFunc);

void ExternalCommandListener::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const ExternalCommandListener::Ptr& externalcommandlistener : ConfigType::GetObjectsByType<ExternalCommandListener>()) {
		nodes->Set(externalcommandlistener->GetName(), 1); //add more stats
	}

	status->Set("externalcommandlistener", nodes);
}

/**
 * Starts the component.
 */
void ExternalCommandListener::Start(bool runtimeCreated)
{
	ObjectImpl<ExternalCommandListener>::Start(runtimeCreated);

	Log(LogInformation, "ExternalCommandListener")
	    << "'" << GetName() << "' started.";

#ifndef _WIN32
	m_CommandThread = std::thread(std::bind(&ExternalCommandListener::CommandPipeThread, this, GetCommandPath()));
	m_CommandThread.detach();
#endif /* _WIN32 */
}

/**
 * Stops the component.
 */
void ExternalCommandListener::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "ExternalCommandListener")
	    << "'" << GetName() << "' stopped.";

	ObjectImpl<ExternalCommandListener>::Stop(runtimeRemoved);
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
		Log(LogCritical, "ExternalCommandListener")
		    << "mkfifo() for fifo path '" << commandPath << "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
		return;
	}

	/* mkfifo() uses umask to mask off some bits, which means we need to chmod() the
	 * fifo to get the right mask. */
	if (chmod(commandPath.CStr(), mode) < 0) {
		Log(LogCritical, "ExternalCommandListener")
		    << "chmod() on fifo '" << commandPath << "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
		return;
	}

	for (;;) {
		int fd = open(commandPath.CStr(), O_RDWR | O_NONBLOCK);

		if (fd < 0) {
			Log(LogCritical, "ExternalCommandListener")
			    << "open() for fifo path '" << commandPath << "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			return;
		}

		FIFO::Ptr fifo = new FIFO();
		Socket::Ptr sock = new Socket(fd);
		StreamReadContext src;

		for (;;) {
			sock->Poll(true, false);

			char buffer[8192];
			size_t rc;

			try {
				rc = sock->Read(buffer, sizeof(buffer));
			} catch (const std::exception& ex) {
				/* We have read all data. */
				if (errno == EAGAIN)
					continue;

				Log(LogWarning, "ExternalCommandListener")
				    << "Cannot read from command pipe." << DiagnosticInformation(ex);
				break;
			}

			/* Empty pipe (EOF) */
			if (rc == 0)
				continue;

			fifo->Write(buffer, rc);

			for (;;) {
				String command;
				StreamReadStatus srs = fifo->ReadLine(&command, src);

				if (srs != StatusNewItem)
					break;

				try {
					Log(LogInformation, "ExternalCommandListener")
					    << "Executing external command: " << command;

					ExternalCommandProcessor::Execute(command);
				} catch (const std::exception& ex) {
					Log(LogWarning, "ExternalCommandListener")
					    << "External command failed: " << DiagnosticInformation(ex, false);
					Log(LogNotice, "ExternalCommandListener")
					    << "External command failed: " << DiagnosticInformation(ex, true);
				}
			}
		}
	}
}
#endif /* _WIN32 */
