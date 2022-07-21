/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#include "base/atomic-file.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include <utility>

#ifdef _WIN32
#	include <io.h>
#	include <windows.h>
#else /* _WIN32 */
#	include <errno.h>
#	include <unistd.h>
#endif /* _WIN32 */

using namespace icinga;

AtomicFile::AtomicFile(String path, int mode) : m_Path(std::move(path))
{
	m_TempFilename = m_Path + ".tmp.XXXXXX";

#ifdef _WIN32
	m_Fd = Utility::MksTemp(&m_TempFilename[0]);
#else /* _WIN32 */
	m_Fd = mkstemp(&m_TempFilename[0]);
#endif /* _WIN32 */

	if (m_Fd < 0) {
		auto error (errno);

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("mkstemp")
			<< boost::errinfo_errno(error)
			<< boost::errinfo_file_name(m_TempFilename));
	}

	try {
		exceptions(failbit | badbit);

		open(boost::iostreams::file_descriptor(
			m_Fd,
			// Rationale: https://github.com/boostorg/iostreams/issues/152
			boost::iostreams::never_close_handle
		));

		if (chmod(m_TempFilename.CStr(), mode) < 0) {
			auto error (errno);

			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("chmod")
				<< boost::errinfo_errno(error)
				<< boost::errinfo_file_name(m_TempFilename));
		}
	} catch (...) {
		if (is_open()) {
			close();
		}

		(void)::close(m_Fd);
		(void)unlink(m_TempFilename.CStr());
		throw;
	}
}

AtomicFile::~AtomicFile()
{
	if (is_open()) {
		try {
			close();
		} catch (...) {
			// Destructor must not throw
		}
	}

	if (m_Fd >= 0) {
		(void)::close(m_Fd);
	}

	if (!m_TempFilename.IsEmpty()) {
		(void)unlink(m_TempFilename.CStr());
	}
}

void AtomicFile::Commit()
{
	flush();

	auto h ((*this)->handle());

#ifdef _WIN32
	if (!FlushFileBuffers(h)) {
		auto err (GetLastError());

		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("FlushFileBuffers")
			<< errinfo_win32_error(err)
			<< boost::errinfo_file_name(m_TempFilename));
	}
#else /* _WIN32 */
	if (fsync(h)) {
		auto err (errno);

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("fsync")
			<< boost::errinfo_errno(err)
			<< boost::errinfo_file_name(m_TempFilename));
	}
#endif /* _WIN32 */

	close();
	(void)::close(m_Fd);
	m_Fd = -1;

	Utility::RenameFile(m_TempFilename, m_Path);
	m_TempFilename = "";
}
