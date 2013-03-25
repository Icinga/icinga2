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

#ifndef COMPATLOG_H
#define COMPATLOG_H

#include "remoting/endpoint.h"
#include "base/dynamicobject.h"
#include "base/timer.h"
#include <fstream>

namespace icinga
{

/**
 * An Icinga compat log writer.
 *
 * @ingroup compat
 */
class CompatLog : public DynamicObject
{
public:
	typedef shared_ptr<CompatLog> Ptr;
	typedef weak_ptr<CompatLog> WeakPtr;

	CompatLog(const Dictionary::Ptr& serializedUpdate);

	static CompatLog::Ptr GetByName(const String& name);

	String GetLogDir(void) const;
	String GetRotationMethod(void) const;

	static Value ValidateRotationMethod(const std::vector<Value>& arguments);

protected:
	virtual void OnAttributeChanged(const String& name);
	virtual void Start(void);

private:
	Attribute<String> m_LogDir;
	Attribute<String> m_RotationMethod;

	double m_LastRotation;

	void WriteLine(const String& line);
	void Flush(void);

	Endpoint::Ptr m_Endpoint;
	void CheckResultRequestHandler(const RequestMessage& request);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler(void);
	void ScheduleNextRotation(void);

	std::ofstream m_OutputFile;
	void ReopenFile(bool rotate);
};

}

#endif /* COMPATLOG_H */
