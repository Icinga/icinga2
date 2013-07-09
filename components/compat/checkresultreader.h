/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef CHECKRESULTREADER_H
#define CHECKRESULTREADER_H

#include "remoting/endpoint.h"
#include "base/dynamicobject.h"
#include "base/timer.h"
#include <fstream>

namespace icinga
{

/**
 * An Icinga checkresult reader.
 *
 * @ingroup compat
 */
class CheckResultReader : public DynamicObject
{
public:
	DECLARE_PTR_TYPEDEFS(CheckResultReader);

	CheckResultReader(const Dictionary::Ptr& properties);

	static CheckResultReader::Ptr GetByName(const String& name);

	String GetSpoolDir(void) const;

protected:
	virtual void Start(void);

private:
	Attribute<String> m_SpoolDir;

	Timer::Ptr m_ReadTimer;
	void ReadTimerHandler(void) const;
	void ProcessCheckResultFile(const String& path) const;
};

}

#endif /* CHECKRESULTREADER_H */
