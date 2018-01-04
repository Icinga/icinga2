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

#ifndef STATUSTABLE_H
#define STATUSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class StatusTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(StatusTable);

	StatusTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const override;
	virtual String GetPrefix(void) const override;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn) override;

	static Value ConnectionsAccessor(const Value& row);
	static Value ConnectionsRateAccessor(const Value& row);
	static Value ServiceChecksAccessor(const Value& row);
	static Value ServiceChecksRateAccessor(const Value& row);
	static Value HostChecksAccessor(const Value& row);
	static Value HostChecksRateAccessor(const Value& row);
	static Value ExternalCommandsAccessor(const Value& row);
	static Value ExternalCommandsRateAccessor(const Value& row);
	static Value NagiosPidAccessor(const Value& row);
	static Value EnableNotificationsAccessor(const Value& row);
	static Value ExecuteServiceChecksAccessor(const Value& row);
	static Value ExecuteHostChecksAccessor(const Value& row);
	static Value EnableEventHandlersAccessor(const Value& row);
	static Value EnableFlapDetectionAccessor(const Value& row);
	static Value ProcessPerformanceDataAccessor(const Value& row);
	static Value ProgramStartAccessor(const Value& row);
	static Value NumHostsAccessor(const Value& row);
	static Value NumServicesAccessor(const Value& row);
	static Value ProgramVersionAccessor(const Value& row);
	static Value LivestatusVersionAccessor(const Value& row);
	static Value LivestatusActiveConnectionsAccessor(const Value& row);
	static Value CustomVariableNamesAccessor(const Value& row);
	static Value CustomVariableValuesAccessor(const Value& row);
	static Value CustomVariablesAccessor(const Value& row);
};

}

#endif /* STATUSTABLE_H */
