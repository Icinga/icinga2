/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "compat/externalcommandlistener.hpp"
#include "compat/externalcommandlistener-ti.cpp"
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
	DictionaryData nodes;

	for (const auto& externalcommandlistener : ConfigType::GetObjectsByType<ExternalCommandListener>()) {
		nodes.emplace_back(externalcommandlistener->GetName(), 1); //add more stats
	}

	status->Set("externalcommandlistener", new Dictionary(std::move(nodes)));
}

/**
 * Starts the component.
 */
void ExternalCommandListener::Start(bool runtimeCreated)
{
	ObjectImpl<ExternalCommandListener>::Start(runtimeCreated);

	Log(LogInformation, "ExternalCommandListener")
		<< "'" << GetName() << "' started.";

	Log(LogWarning, "ExternalCommandListener")
		<< "This feature is DEPRECATED and may be removed in future releases. Check the roadmap at https://github.com/Icinga/icinga2/milestones";
#ifndef _WIN32
	String path = GetCommandPath();
	m_CommandThread = std::thread([this, path]() { CommandPipeThread(path); });
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
			Utility::Remove(commandPath);
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
