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

#ifndef STATUSTABLE_H
#define STATUSTABLE_H

#include "livestatus/table.h"

using namespace icinga;

namespace livestatus
{

/**
 * @ingroup livestatus
 */
class StatusTable : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(StatusTable);

	StatusTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn);

	static Value NebCallbacksAccessor(const Value& row);
	static Value NebCallbacksRateAccessor(const Value& row);
	static Value RequestsAccessor(const Value& row);
	static Value RequestsRateAccessor(const Value& row);
	static Value ConnectionsAccessor(const Value& row);
	static Value ConnectionsRateAccessor(const Value& row);
	static Value ServiceChecksAccessor(const Value& row);
	static Value ServiceChecksRateAccessor(const Value& row);
	static Value HostChecksAccessor(const Value& row);
	static Value HostChecksRateAccessor(const Value& row);
	static Value ForksAccessor(const Value& row);
	static Value ForksRateAccessor(const Value& row);
	static Value LogMessagesAccessor(const Value& row);
	static Value LogMessagesRateAccessor(const Value& row);
	static Value ExternalCommandsAccessor(const Value& row);
	static Value ExternalCommandsRateAccessor(const Value& row);
	static Value LivechecksAccessor(const Value& row);
	static Value LivechecksRateAccessor(const Value& row);
	static Value LivecheckOverflowsAccessor(const Value& row);
	static Value LivecheckOverflowsRateAccessor(const Value& row);
	static Value NagiosPidAccessor(const Value& row);
	static Value EnableNotificationsAccessor(const Value& row);
	static Value ExecuteServiceChecksAccessor(const Value& row);
	static Value AcceptPassiveServiceChecksAccessor(const Value& row);
	static Value ExecuteHostChecksAccessor(const Value& row);
	static Value AcceptPassiveHostChecksAccessor(const Value& row);
	static Value EnableEventHandlersAccessor(const Value& row);
	static Value ObsessOverServicesAccessor(const Value& row);
	static Value ObsessOverHostsAccessor(const Value& row);
	static Value CheckServiceFreshnessAccessor(const Value& row);
	static Value CheckHostFreshnessAccessor(const Value& row);
	static Value EnableFlapDetectionAccessor(const Value& row);
	static Value ProcessPerformanceDataAccessor(const Value& row);
	static Value CheckExternalCommandsAccessor(const Value& row);
	static Value ProgramStartAccessor(const Value& row);
	static Value LastCommandCheckAccessor(const Value& row);
	static Value LastLogRotationAccessor(const Value& row);
	static Value IntervalLengthAccessor(const Value& row);
	static Value NumHostsAccessor(const Value& row);
	static Value NumServicesAccessor(const Value& row);
	static Value ProgramVersionAccessor(const Value& row);
	static Value ExternalCommandBufferSlotsAccessor(const Value& row);
	static Value ExternalCommandBufferUsageAccessor(const Value& row);
	static Value ExternalCommandBufferMaxAccessor(const Value& row);
	static Value CachedLogMessagesAccessor(const Value& row);
	static Value LivestatusVersionAccessor(const Value& row);
	static Value LivestatusActiveConnectionsAccessor(const Value& row);
	static Value LivestatusQueuedConnectionsAccessor(const Value& row);
	static Value LivestatusThreadsAccessor(const Value& row);
};

}

#endif /* STATUSTABLE_H */
