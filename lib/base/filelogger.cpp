/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/filelogger.hpp"
#include "base/filelogger-ti.cpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include "base/application.hpp"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(FileLogger);

REGISTER_STATSFUNCTION(FileLogger, &FileLogger::StatsFunc);

void FileLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	DictionaryData nodes;

	for (const FileLogger::Ptr& filelogger : ConfigType::GetObjectsByType<FileLogger>()) {
		nodes.emplace_back(filelogger->GetName(), 1); //add more stats
	}

	status->Set("filelogger", new Dictionary(std::move(nodes)));
}

/**
 * Constructor for the FileLogger class.
 */
void FileLogger::Start(bool runtimeCreated)
{
	ReopenLogFile();

	Application::OnReopenLogs.connect([this]() { ReopenLogFile(); });

	ObjectImpl<FileLogger>::Start(runtimeCreated);

	Log(LogInformation, "FileLogger")
		<< "'" << GetName() << "' started.";
}

void FileLogger::ReopenLogFile()
{
	auto *stream = new std::ofstream();

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
