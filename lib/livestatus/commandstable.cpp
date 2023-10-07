/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/commandstable.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/compatutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

CommandsTable::CommandsTable()
{
	AddColumns(this);
}

void CommandsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&CommandsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "line", Column(&CommandsTable::LineAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_names", Column(&CommandsTable::CustomVariableNamesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&CommandsTable::CustomVariableValuesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&CommandsTable::CustomVariablesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::ZeroAccessor, objectAccessor));
}

String CommandsTable::GetName() const
{
	return "commands";
}

String CommandsTable::GetPrefix() const
{
	return "command";
}

void CommandsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (auto& object : ConfigType::GetObjectsByType<CheckCommand>()) {
		if (!addRowFn(object, LivestatusGroupByNone, Empty))
			return;
	}

	for (auto& object : ConfigType::GetObjectsByType<EventCommand>()) {
		if (!addRowFn(object, LivestatusGroupByNone, Empty))
			return;
	}

	for (auto& object : ConfigType::GetObjectsByType<NotificationCommand>()) {
		if (!addRowFn(object, LivestatusGroupByNone, Empty))
			return;
	}
}

Value CommandsTable::NameAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	return CompatUtility::GetCommandName(command);
}

Value CommandsTable::LineAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	return CompatUtility::GetCommandLine(command);
}

Value CommandsTable::CustomVariableNamesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	Dictionary::Ptr vars = command->GetVars();

	ArrayData keys;

	if (vars) {
		ObjectLock xlock(vars);
		for (const auto& kv : vars) {
			keys.push_back(kv.first);
		}
	}

	return new Array(std::move(keys));
}

Value CommandsTable::CustomVariableValuesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	Dictionary::Ptr vars = command->GetVars();

	ArrayData keys;

	if (vars) {
		ObjectLock xlock(vars);
		for (const auto& kv : vars) {
			keys.push_back(kv.second);
		}
	}

	return new Array(std::move(keys));
}

Value CommandsTable::CustomVariablesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	Dictionary::Ptr vars = command->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock xlock(vars);
		for (const auto& kv : vars) {
			result.push_back(new Array({
				kv.first,
				kv.second
			}));
		}
	}

	return new Array(std::move(result));
}
