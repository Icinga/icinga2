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

#include "livestatus/table.h"
#include "livestatus/statustable.h"
#include "livestatus/contactgroupstable.h"
#include "livestatus/contactstable.h"
#include "livestatus/hostgroupstable.h"
#include "livestatus/hoststable.h"
#include "livestatus/servicegroupstable.h"
#include "livestatus/servicestable.h"
#include "livestatus/commandstable.h"
#include "livestatus/commentstable.h"
#include "livestatus/downtimestable.h"
#include "livestatus/timeperiodstable.h"
#include "livestatus/logtable.h"
#include "livestatus/filter.h"
#include "base/array.h"
#include "base/dictionary.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

using namespace icinga;
using namespace livestatus;

Table::Table(void)
{ }

Table::Ptr Table::GetByName(const String& name)
{
	if (name == "status")
		return boost::make_shared<StatusTable>();
	else if (name == "contactgroups")
		return boost::make_shared<ContactGroupsTable>();
	else if (name == "contacts")
		return boost::make_shared<ContactsTable>();
	else if (name == "hostgroups")
		return boost::make_shared<HostGroupsTable>();
	else if (name == "hosts")
		return boost::make_shared<HostsTable>();
	else if (name == "servicegroups")
		return boost::make_shared<ServiceGroupsTable>();
	else if (name == "services")
		return boost::make_shared<ServicesTable>();
	else if (name == "commands")
		return boost::make_shared<CommandsTable>();
	else if (name == "comments")
		return boost::make_shared<CommentsTable>();
	else if (name == "downtimes")
		return boost::make_shared<DowntimesTable>();
	else if (name == "timeperiods")
		return boost::make_shared<TimePeriodsTable>();
	else if (name == "log")
		return boost::make_shared<LogTable>();

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
	std::map<String, Column>::const_iterator it = m_Columns.find(name);

	if (it == m_Columns.end())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Column '" + name + "' does not exist in table '" + GetName() + "'."));

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
	if (!filter || filter->Apply(GetSelf(), row))
		rs.push_back(row);
}

Value Table::ZeroAccessor(const Object::Ptr&)
{
	return 0;
}

Value Table::OneAccessor(const Object::Ptr&)
{
	return 1;
}

Value Table::EmptyStringAccessor(const Object::Ptr&)
{
	return "";
}

Value Table::EmptyArrayAccessor(const Object::Ptr&)
{
	return boost::make_shared<Array>();
}

Value Table::EmptyDictionaryAccessor(const Object::Ptr&)
{
	return boost::make_shared<Dictionary>();
}
