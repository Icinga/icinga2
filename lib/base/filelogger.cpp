/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/exception.hpp"
#include "base/filelogger.hpp"
#include "base/filelogger-ti.cpp"
#include "base/configtype.hpp"
#include "base/convert.hpp"
#include "base/statsfunction.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <errno.h>
#include <fstream>
#include <sys/stat.h>

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

	Application::OnReopenLogs.connect(std::bind(&FileLogger::ReopenLogFile, this));

	ObjectImpl<FileLogger>::Start(runtimeCreated);

	Log(LogInformation, "FileLogger")
		<< "'" << GetName() << "' started.";
}

#ifdef _WIN32
#	define stat _stat
#endif /* _WIN32 */

void FileLogger::Flush()
{
	namespace fs = boost::filesystem;
	namespace sys = boost::system;

	StreamLogger::Flush();

	auto maxSize (GetMaxSize());

	if (maxSize > -1) {
		struct stat st;
		auto path (GetPath());

		if (stat(path.CStr(), &st)) {
			if (errno == ENOENT) {
				return;
			}

			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("stat")
				<< boost::errinfo_errno(errno)
				<< boost::errinfo_file_name(path));
		}

		if (st.st_size >= maxSize) {
			auto keepRotations (GetKeepRotations());
			ObjectLock oLock (this);

			BindStream(nullptr, false);

			if (keepRotations > 0) {
				Utility::Remove(path + "." + Convert::ToString(keepRotations));

				for (auto i (keepRotations); i > 1;) {
					auto to (path + "." + Convert::ToString(i));

					try {
						Utility::RenameFile(path + "." + Convert::ToString(--i), to);
					} catch (const fs::filesystem_error& ex) {
						if (ex.code() != sys::error_code(ENOENT, sys::system_category())) {
							throw;
						}
					}
				}

				try {
					Utility::RenameFile(path, path + ".1");
				} catch (const fs::filesystem_error& ex) {
					if (ex.code() != sys::error_code(ENOENT, sys::system_category())) {
						throw;
					}
				}
			} else {
				Utility::Remove(path);
			}

			ReopenLogFile();
		}
	}
}

#ifdef _WIN32
#	undef stat
#endif /* _WIN32 */

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
