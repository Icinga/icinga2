/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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

	StatusTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

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
	static Value IntervalLengthAccessor(const Value& row);
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
