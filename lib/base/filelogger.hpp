/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef FILELOGGER_H
#define FILELOGGER_H

#include "base/i2-base.hpp"
#include "base/filelogger-ti.hpp"

namespace icinga
{

/**
 * A logger that logs to a file.
 *
 * @ingroup base
 */
class FileLogger : public ObjectImpl<FileLogger>
{
public:
	DECLARE_OBJECT(FileLogger);
	DECLARE_OBJECTNAME(FileLogger);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void Start(bool runtimeCreated) override;

private:
	void ReopenLogFile();
};

}

#endif /* FILELOGGER_H */
