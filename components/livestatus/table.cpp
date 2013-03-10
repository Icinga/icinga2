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

#include "i2-livestatus.h"

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
	else if (name == "hosts")
		return boost::make_shared<HostsTable>();

	return Table::Ptr();
}

void Table::AddColumn(const String& name, const Column& column)
{
	pair<String, Column> item = make_pair(name, column);

	pair<map<String, Column>::iterator, bool> ret = m_Columns.insert(item);

	if (!ret.second)
		ret.first->second = column;
}

Column Table::GetColumn(const String& name) const
{
	map<String, Column>::const_iterator it = m_Columns.find(name);

	if (it == m_Columns.end())
		BOOST_THROW_EXCEPTION(invalid_argument("Column '" + name + "' does not exist."));

	return it->second;
}

vector<String> Table::GetColumnNames(void) const
{
	vector<String> names;

	String name;
	BOOST_FOREACH(tie(name, tuples::ignore), m_Columns) {
		names.push_back(name);
	}

	return names;
}

vector<Object::Ptr> Table::FilterRows(const Filter::Ptr& filter)
{
	vector<Object::Ptr> rs;

	FetchRows(boost::bind(&Table::FilteredAddRow, this, boost::ref(rs), filter, _1));

	return rs;
}

void Table::FilteredAddRow(vector<Object::Ptr>& rs, const Filter::Ptr& filter, const Object::Ptr& object)
{
	if (!filter || filter->Apply(GetSelf(), object))
		rs.push_back(object);
}

Value Table::ZeroAccessor(const Object::Ptr&)
{
	return 0;
}

Value Table::OneAccessor(const Object::Ptr&)
{
	return 0;
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
