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

CommentsTable::CommentsTable(void)
{
	AddColumns(this);
}

void CommentsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "author", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "comment", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "id", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_time", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "type", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "is_service", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "persistent", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "source", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_type", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "expires", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "expire_time", Column(&Table::EmptyStringAccessor, objectAccessor));

	// TODO: Join services table.
}

String CommentsTable::GetName(void) const
{
	return "comments";
}

void CommentsTable::FetchRows(const function<void (const Object::Ptr&)>& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);
		Dictionary::Ptr comments = service->GetComments();

		if (!comments)
			continue;

		ObjectLock olock(comments);

		Value comment;
		BOOST_FOREACH(tie(tuples::ignore, comment), comments) {
			addRowFn(comment);
		}
	}
}
