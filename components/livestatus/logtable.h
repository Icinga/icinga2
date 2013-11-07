/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef LOGTABLE_H
#define LOGTABLE_H

#include "livestatus/table.h"
#include <boost/thread/mutex.hpp>

using namespace icinga;

namespace livestatus
{

enum LogType {
    LogTypeHostAlert,
    LogTypeHostDowntimeAlert,
    LogTypeHostFlapping,
    LogTypeHostNotification,
    LogTypeHostInitialState,
    LogTypeHostCurrentState,
    LogTypeServiceAlert,
    LogTypeServiceDowntimeAlert,
    LogTypeServiceFlapping,
    LogTypeServiceNotification,
    LogTypeServiceInitialState,
    LogTypeServiceCurrentState,
    LogTypeTimeperiodTransition,
    LogTypeVersion,
    LogTypeInitialStates,
    LogTypeProgramStarting
};

enum LogClass {
    LogClassInfo = 0,
    LogClassAlert = 1,
    LogClassProgram = 2,
    LogClassNotification = 3,
    LogClassPassive = 4,
    LogClassCommand = 5,
    LogClassState = 6,
    LogClassText = 7
};

/**
 * @ingroup livestatus
 */
class LogTable : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(LogTable);

	LogTable(const String& compat_log_path, const unsigned long& from, const unsigned long& until);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn);

        static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
        static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
        static Object::Ptr ContactAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
        static Object::Ptr CommandAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value TimeAccessor(const Value& row);
	static Value LinenoAccessor(const Value& row);
	static Value ClassAccessor(const Value& row);
	static Value MessageAccessor(const Value& row);
	static Value TypeAccessor(const Value& row);
	static Value OptionsAccessor(const Value& row);
	static Value CommentAccessor(const Value& row);
	static Value PluginOutputAccessor(const Value& row);
	static Value StateAccessor(const Value& row);
	static Value StateTypeAccessor(const Value& row);
	static Value AttemptAccessor(const Value& row);
	static Value ServiceDescriptionAccessor(const Value& row);
	static Value HostNameAccessor(const Value& row);
	static Value ContactNameAccessor(const Value& row);
	static Value CommandNameAccessor(const Value& row);
        
private:
        std::map<unsigned long, String> m_LogFileIndex;
        std::map<unsigned long, Dictionary::Ptr> m_RowsCache;
        unsigned long m_TimeFrom;
        unsigned long m_TimeUntil;
        boost::mutex m_Mutex;
        
        void CreateLogIndex(const String& path);
        static void CreateLogIndexFileHandler(const String& path, std::map<unsigned long, String>& index);
        void GetLogClassType(const String& text, int& log_class, int& log_type);
        Dictionary::Ptr GetLogEntryAttributes(const String& type, const String& options);
};

}

#endif /* LOGTABLE_H */
