// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
class FileLogger final : public ObjectImpl<FileLogger>
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
