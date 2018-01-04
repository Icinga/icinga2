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

#include "livestatus/table.hpp"
#include "livestatus/statustable.hpp"
#include "livestatus/contactgroupstable.hpp"
#include "livestatus/contactstable.hpp"
#include "livestatus/hostgroupstable.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicegroupstable.hpp"
#include "livestatus/servicestable.hpp"
#include "livestatus/commandstable.hpp"
#include "livestatus/commentstable.hpp"
#include "livestatus/downtimestable.hpp"
#include "livestatus/endpointstable.hpp"
#include "livestatus/zonestable.hpp"
#include "livestatus/timeperiodstable.hpp"
#include "livestatus/logtable.hpp"
#include "livestatus/statehisttable.hpp"
#include "livestatus/filter.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

Table::Table(LivestatusGroupByType type)
	: m_GroupByType(type), m_GroupByObject(Empty)
{ }

Table::Ptr Table::GetByName(const String& name, const String& compat_log_path, const unsigned long& from, const unsigned long& until)
{
	if (name == "status")
		return new StatusTable();
	else if (name == "contactgroups")
		return new ContactGroupsTable();
	else if (name == "contacts")
		return new ContactsTable();
	else if (name == "hostgroups")
		return new HostGroupsTable();
	else if (name == "hosts")
		return new HostsTable();
	else if (name == "hostsbygroup")
		return new HostsTable(LivestatusGroupByHostGroup);
	else if (name == "servicegroups")
		return new ServiceGroupsTable();
	else if (name == "services")
		return new ServicesTable();
	else if (name == "servicesbygroup")
		return new ServicesTable(LivestatusGroupByServiceGroup);
	else if (name == "servicesbyhostgroup")
		return new ServicesTable(LivestatusGroupByHostGroup);
	else if (name == "commands")
		return new CommandsTable();
	else if (name == "comments")
		return new CommentsTable();
	else if (name == "downtimes")
		return new DowntimesTable();
	else if (name == "timeperiods")
		return new TimePeriodsTable();
	else if (name == "log")
		return new LogTable(compat_log_path, from, until);
	else if (name == "statehist")
		return new StateHistTable(compat_log_path, from, until);
	else if (name == "endpoints")
		return new EndpointsTable();
	else if (name == "zones")
		return new ZonesTable();

	return nullptr;
}

void Table::AddColumn(const String& name, const Column& column)
{
	std::pair<String, Column> item = std::make_pair(name, column);

	auto ret = m_Columns.insert(item);

	if (!ret.second)
		ret.first->second = column;
}

Column Table::GetColumn(const String& name) const
{
	String dname = name;
	String prefix = GetPrefix() + "_";

	if (dname.Find(prefix) == 0)
		dname = dname.SubStr(prefix.GetLength());

	auto it = m_Columns.find(dname);

	if (it == m_Columns.end())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Column '" + dname + "' does not exist in table '" + GetName() + "'."));

	return it->second;
}

std::vector<String> Table::GetColumnNames() const
{
	std::vector<String> names;

	for (const auto& kv : m_Columns) {
		names.push_back(kv.first);
	}

	return names;
}

std::vector<LivestatusRowValue> Table::FilterRows(const Filter::Ptr& filter, int limit)
{
	std::vector<LivestatusRowValue> rs;

	FetchRows(std::bind(&Table::FilteredAddRow, this, std::ref(rs), filter, limit, _1, _2, _3));

	return rs;
}

bool Table::FilteredAddRow(std::vector<LivestatusRowValue>& rs, const Filter::Ptr& filter, int limit, const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject)
{
	if (limit != -1 && static_cast<int>(rs.size()) == limit)
		return false;

	if (!filter || filter->Apply(this, row)) {
		LivestatusRowValue rval;
		rval.Row = row;
		rval.GroupByType = groupByType;
		rval.GroupByObject = groupByObject;

		rs.emplace_back(std::move(rval));
	}

	return true;
}

Value Table::ZeroAccessor(const Value&)
{
	return 0;
}

Value Table::OneAccessor(const Value&)
{
	return 1;
}

Value Table::EmptyStringAccessor(const Value&)
{
	return "";
}

Value Table::EmptyArrayAccessor(const Value&)
{
	return new Array();
}

Value Table::EmptyDictionaryAccessor(const Value&)
{
	return new Dictionary();
}

LivestatusGroupByType Table::GetGroupByType() const
{
	return m_GroupByType;
}
