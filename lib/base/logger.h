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

#ifndef LOGGER_H
#define LOGGER_H

#include "base/i2-base.h"
#include "base/dynamicobject.h"
#include "base/logger_fwd.h"

namespace icinga
{

/**
 * A lot entry.
 *
 * @ingroup base
 */
struct LogEntry {
	double Timestamp; /**< The timestamp when this log entry was created. */
	LogSeverity Severity; /**< The severity of this log entry. */
	String Facility; /**< The facility this log entry belongs to. */
	String Message; /**< The log entry's message. */
};

/**
 * Base class for all loggers.
 *
 * @ingroup base
 */
class I2_BASE_API ILogger : public Object
{
public:
	typedef shared_ptr<ILogger> Ptr;
	typedef weak_ptr<ILogger> WeakPtr;

	/**
	 * Processes the log entry and writes it to the log that is
	 * represented by this ILogger object.
	 *
	 * @param entry The log entry that is to be processed.
	 */
	virtual void ProcessLogEntry(const LogEntry& entry) = 0;

protected:
	DynamicObject::Ptr GetConfig(void) const;

private:
	DynamicObject::WeakPtr m_Config;

	friend class Logger;
};

/**
 * A log provider. Can be instantiated from the config.
 *
 * @ingroup base
 */
class I2_BASE_API Logger : public DynamicObject
{
public:
	typedef shared_ptr<Logger> Ptr;
	typedef weak_ptr<Logger> WeakPtr;

	explicit Logger(const Dictionary::Ptr& serializedUpdate);

	static String SeverityToString(LogSeverity severity);
	static LogSeverity StringToSeverity(const String& severity);

	LogSeverity GetMinSeverity(void) const;

protected:
	virtual void Start(void);

private:
	Attribute<String> m_Type;
	Attribute<String> m_Path;
	Attribute<String> m_Severity;

	LogSeverity m_MinSeverity;
	ILogger::Ptr m_Impl;

	static void ForwardLogEntry(const LogEntry& entry);

	friend void Log(LogSeverity severity, const String& facility,
	    const String& message);
};

}

#endif /* LOGGER_H */
