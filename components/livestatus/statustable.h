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

	static Value NebCallbacksAccessor(const Object::Ptr& object);
	static Value NebCallbacksRateAccessor(const Object::Ptr& object);
	static Value RequestsAccessor(const Object::Ptr& object);
	static Value RequestsRateAccessor(const Object::Ptr& object);
	static Value ConnectionsAccessor(const Object::Ptr& object);
	static Value ConnectionsRateAccessor(const Object::Ptr& object);
	static Value ServiceChecksAccessor(const Object::Ptr& object);
	static Value ServiceChecksRateAccessor(const Object::Ptr& object);
	static Value HostChecksAccessor(const Object::Ptr& object);
	static Value HostChecksRateAccessor(const Object::Ptr& object);
	static Value ForksAccessor(const Object::Ptr& object);
	static Value ForksRateAccessor(const Object::Ptr& object);
	static Value LogMessagesAccessor(const Object::Ptr& object);
	static Value LogMessagesRateAccessor(const Object::Ptr& object);
	static Value ExternalCommandsAccessor(const Object::Ptr& object);
	static Value ExternalCommandsRateAccessor(const Object::Ptr& object);
	static Value LivechecksAccessor(const Object::Ptr& object);
	static Value LivechecksRateAccessor(const Object::Ptr& object);
	static Value LivecheckOverflowsAccessor(const Object::Ptr& object);
	static Value LivecheckOverflowsRateAccessor(const Object::Ptr& object);
	static Value NagiosPidAccessor(const Object::Ptr& object);
	static Value EnableNotificationsAccessor(const Object::Ptr& object);
	static Value ExecuteServiceChecksAccessor(const Object::Ptr& object);
	static Value AcceptPassiveServiceChecksAccessor(const Object::Ptr& object);
	static Value ExecuteHostChecksAccessor(const Object::Ptr& object);
	static Value AcceptPassiveHostChecksAccessor(const Object::Ptr& object);
	static Value EnableEventHandlersAccessor(const Object::Ptr& object);
	static Value ObsessOverServicesAccessor(const Object::Ptr& object);
	static Value ObsessOverHostsAccessor(const Object::Ptr& object);
	static Value CheckServiceFreshnessAccessor(const Object::Ptr& object);
	static Value CheckHostFreshnessAccessor(const Object::Ptr& object);
	static Value EnableFlapDetectionAccessor(const Object::Ptr& object);
	static Value ProcessPerformanceDataAccessor(const Object::Ptr& object);
	static Value CheckExternalCommandsAccessor(const Object::Ptr& object);
	static Value ProgramStartAccessor(const Object::Ptr& object);
	static Value LastCommandCheckAccessor(const Object::Ptr& object);
	static Value LastLogRotationAccessor(const Object::Ptr& object);
	static Value IntervalLengthAccessor(const Object::Ptr& object);
	static Value NumHostsAccessor(const Object::Ptr& object);
	static Value NumServicesAccessor(const Object::Ptr& object);
	static Value ProgramVersionAccessor(const Object::Ptr& object);
	static Value ExternalCommandBufferSlotsAccessor(const Object::Ptr& object);
	static Value ExternalCommandBufferUsageAccessor(const Object::Ptr& object);
	static Value ExternalCommandBufferMaxAccessor(const Object::Ptr& object);
	static Value CachedLogMessagesAccessor(const Object::Ptr& object);
	static Value LivestatusVersionAccessor(const Object::Ptr& object);
	static Value LivestatusActiveConnectionsAccessor(const Object::Ptr& object);
	static Value LivestatusQueuedConnectionsAccessor(const Object::Ptr& object);
	static Value LivestatusThreadsAccessor(const Object::Ptr& object);
};

}

#endif /* STATUSTABLE_H */
