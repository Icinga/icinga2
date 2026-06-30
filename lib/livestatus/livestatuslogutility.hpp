// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LIVESTATUSLOGUTILITY_H
#define LIVESTATUSLOGUTILITY_H

#include "livestatus/historytable.hpp"

using namespace icinga;

namespace icinga
{

enum LogEntryType {
	LogEntryTypeHostAlert,
	LogEntryTypeHostDowntimeAlert,
	LogEntryTypeHostFlapping,
	LogEntryTypeHostNotification,
	LogEntryTypeHostInitialState,
	LogEntryTypeHostCurrentState,
	LogEntryTypeServiceAlert,
	LogEntryTypeServiceDowntimeAlert,
	LogEntryTypeServiceFlapping,
	LogEntryTypeServiceNotification,
	LogEntryTypeServiceInitialState,
	LogEntryTypeServiceCurrentState,
	LogEntryTypeTimeperiodTransition,
	LogEntryTypeVersion,
	LogEntryTypeInitialStates,
	LogEntryTypeProgramStarting
};

enum LogEntryClass {
	LogEntryClassInfo = 0,
	LogEntryClassAlert = 1,
	LogEntryClassProgram = 2,
	LogEntryClassNotification = 3,
	LogEntryClassPassive = 4,
	LogEntryClassCommand = 5,
	LogEntryClassState = 6,
	LogEntryClassText = 7
};

/**
 * @ingroup livestatus
 */
class LivestatusLogUtility
{
public:
	static void CreateLogIndex(const String& path, std::map<time_t, String>& index);
	static void CreateLogIndexFileHandler(const String& path, std::map<time_t, String>& index);
	static void CreateLogCache(std::map<time_t, String> index, HistoryTable *table, time_t from, time_t until, const AddRowFunction& addRowFn);
	static Dictionary::Ptr GetAttributes(const String& text);

private:
	LivestatusLogUtility();
};

}

#endif /* LIVESTATUSLOGUTILITY_H */
