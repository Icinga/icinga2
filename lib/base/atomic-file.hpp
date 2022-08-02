/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#ifndef ATOMIC_FILE_H
#define ATOMIC_FILE_H

#include "base/string.hpp"
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

namespace icinga
{

/**
 * Atomically replaces a file's content.
 *
 * @ingroup base
 */
class AtomicFile : public boost::iostreams::stream<boost::iostreams::file_descriptor>
{
public:
	static void Write(String path, int mode, const String& content);

	AtomicFile(String path, int mode);
	~AtomicFile();

	inline const String& GetTempFilename() const noexcept
	{
		return m_TempFilename;
	}

	void Commit();

private:
	String m_Path;
	String m_TempFilename;
	int m_Fd;
};

}

#endif /* ATOMIC_FILE_H */
