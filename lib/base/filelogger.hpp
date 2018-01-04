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

#ifndef FILELOGGER_H
#define FILELOGGER_H

#include "base/i2-base.hpp"
#include "base/filelogger.thpp"

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

	virtual void Start(bool runtimeCreated) override;

private:
	void ReopenLogFile(void);
};

}

#endif /* FILELOGGER_H */
