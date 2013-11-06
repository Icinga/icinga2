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

#ifndef STATEHISTTABLE_H
#define STATEHISTTABLE_H

#include "icinga/service.h"
#include "livestatus/table.h"
#include <boost/thread/mutex.hpp>

using namespace icinga;

namespace livestatus
{

enum LogStateHistType {
    LogStateHistTypeHostAlert,
    LogStateHistTypeHostDowntimeAlert,
    LogStateHistTypeHostFlapping,
    LogStateHistTypeHostNotification,
    LogStateHistTypeHostInitialState,
    LogStateHistTypeHostCurrentState,
    LogStateHistTypeServiceAlert,
    LogStateHistTypeServiceDowntimeAlert,
    LogStateHistTypeServiceFlapping,
    LogStateHistTypeServiceNotification,
    LogStateHistTypeServiceInitialState,
    LogStateHistTypeServiceCurrentState,
    LogStateHistTypeTimeperiodTransition,
    LogStateHistTypeVersion,
    LogStateHistTypeInitialStates,
    LogStateHistTypeProgramStarting
};

enum LogStateHistClass {
    LogStateHistClassInfo = 0,
    LogStateHistClassAlert = 1,
    LogStateHistClassProgram = 2,
    LogStateHistClassNotification = 3,
    LogStateHistClassPassive = 4,
    LogStateHistClassCommand = 5,
    LogStateHistClassState = 6,
    LogStateHistClassText = 7
};

/**
 * @ingroup livestatus
 */
class StateHistTable : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(StateHistTable);

	StateHistTable(const unsigned long& from, const unsigned long& until);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn);

        static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
        static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value TimeAccessor(const Value& row);
	static Value LinenoAccessor(const Value& row);
	static Value FromAccessor(const Value& row);
	static Value UntilAccessor(const Value& row);
	static Value DurationAccessor(const Value& row);
	static Value DurationPartAccessor(const Value& row);
	static Value StateAccessor(const Value& row);
	static Value HostDownAccessor(const Value& row);
	static Value InDowntimeAccessor(const Value& row);
	static Value InHostDowntimeAccessor(const Value& row);
	static Value IsFlappingAccessor(const Value& row);
	static Value InNotificationPeriodAccessor(const Value& row);
	static Value NotificationPeriodAccessor(const Value& row);
	static Value HostNameAccessor(const Value& row);
	static Value ServiceDescriptionAccessor(const Value& row);
	static Value LogOutputAccessor(const Value& row);
	static Value DurationOkAccessor(const Value& row);
	static Value DurationPartOkAccessor(const Value& row);
	static Value DurationWarningAccessor(const Value& row);
	static Value DurationPartWarningAccessor(const Value& row);
	static Value DurationCriticalAccessor(const Value& row);
	static Value DurationPartCriticalAccessor(const Value& row);
	static Value DurationUnknownAccessor(const Value& row);
	static Value DurationPartUnknownAccessor(const Value& row);
	static Value DurationUnmonitoredAccessor(const Value& row);
	static Value DurationPartUnmonitoredAccessor(const Value& row);

private:
        std::map<unsigned long, String> m_LogFileIndex;
        std::map<Service::Ptr, Array::Ptr> m_ServicesCache;
        unsigned long m_TimeFrom;
        unsigned long m_TimeUntil;
        boost::mutex m_Mutex;

        void CreateLogIndex(const String& path);
        static void CreateLogIndexFileHandler(const String& path, std::map<unsigned long, String>& index);
        void GetLogStateHistClassType(const String& text, int& log_class, int& log_type);
        Dictionary::Ptr GetStateHistAttributes(const String& type, const String& options);
};

}

#endif /* STATEHISTTABLE_H */
