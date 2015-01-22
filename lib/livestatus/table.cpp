/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "livestatus/timeperiodstable.hpp"
#include "livestatus/logtable.hpp"
#include "livestatus/statehisttable.hpp"
#include "livestatus/filter.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

using namespace icinga;

Table::Table(void)
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
	else if (name == "servicegroups")
		return new ServiceGroupsTable();
	else if (name == "services")
		return new ServicesTable();
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

	return Table::Ptr();
}

void Table::AddColumn(const String& name, const Column& column)
{
	std::pair<String, Column> item = std::make_pair(name, column);

	std::pair<std::map<String, Column>::iterator, bool> ret = m_Columns.insert(item);

	if (!ret.second)
		ret.first->second = column;
}

Column Table::GetColumn(const String& name) const
{
	String dname = name;
	String prefix = GetPrefix() + "_";

	if (dname.Find(prefix) == 0)
		dname = dname.SubStr(prefix.GetLength());

	std::map<String, Column>::const_iterator it = m_Columns.find(dname);

	if (it == m_Columns.end())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Column '" + dname + "' does not exist in table '" + GetName() + "'."));

	return it->second;
}

std::vector<String> Table::GetColumnNames(void) const
{
	std::vector<String> names;

	String name;
	BOOST_FOREACH(boost::tie(name, boost::tuples::ignore), m_Columns) {
		names.push_back(name);
	}

	return names;
}

std::vector<Value> Table::FilterRows(const Filter::Ptr& filter)
{
	std::vector<Value> rs;

	FetchRows(boost::bind(&Table::FilteredAddRow, this, boost::ref(rs), filter, _1));

	return rs;
}

void Table::FilteredAddRow(std::vector<Value>& rs, const Filter::Ptr& filter, const Value& row)
{
	if (!filter || filter->Apply(this, row))
		rs.push_back(row);
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
