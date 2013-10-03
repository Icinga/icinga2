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

#include "livestatus/commentstable.h"
#include "livestatus/servicestable.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

using namespace icinga;
using namespace livestatus;

CommentsTable::CommentsTable(void)
{
	AddColumns(this);
}

void CommentsTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "author", Column(&CommentsTable::AuthorAccessor, objectAccessor));
	table->AddColumn(prefix + "comment", Column(&CommentsTable::CommentAccessor, objectAccessor));
	table->AddColumn(prefix + "id", Column(&CommentsTable::IdAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_time", Column(&CommentsTable::EntryTimeAccessor, objectAccessor));
	table->AddColumn(prefix + "type", Column(&CommentsTable::TypeAccessor, objectAccessor));
	table->AddColumn(prefix + "is_service", Column(&CommentsTable::IsServiceAccessor, objectAccessor));
	table->AddColumn(prefix + "persistent", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "source", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_type", Column(&CommentsTable::EntryTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "expires", Column(&CommentsTable::ExpiresAccessor, objectAccessor));
	table->AddColumn(prefix + "expire_time", Column(&CommentsTable::ExpireTimeAccessor, objectAccessor));

	ServicesTable::AddColumns(table, "service_", boost::bind(&CommentsTable::ServiceAccessor, _1, objectAccessor));
}

String CommentsTable::GetName(void) const
{
	return "comments";
}

void CommentsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const Service::Ptr& service, DynamicType::GetObjects<Service>()) {
		Dictionary::Ptr comments = service->GetComments();

		if (!comments)
			continue;

		ObjectLock olock(comments);

		String id;
		BOOST_FOREACH(boost::tie(id, boost::tuples::ignore), comments) {
			if (Service::GetOwnerByCommentID(id) == service)
				addRowFn(id);
		}
	}
}

Object::Ptr CommentsTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	return Service::GetOwnerByCommentID(row);
}

Value CommentsTable::AuthorAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return comment->Get("author");
}

Value CommentsTable::CommentAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return comment->Get("text");
}

Value CommentsTable::IdAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return comment->Get("legacy_id");
}

Value CommentsTable::EntryTimeAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return static_cast<int>(comment->Get("entry_time"));
}

Value CommentsTable::TypeAccessor(const Value& row)
{
	Service::Ptr svc = Service::GetOwnerByCommentID(row);

	if (!svc)
		return Empty;

	return (svc->IsHostCheck() ? 1 : 2);
}

Value CommentsTable::IsServiceAccessor(const Value& row)
{
	Service::Ptr svc = Service::GetOwnerByCommentID(row);

	if (!svc)
		return Empty;

	return (svc->IsHostCheck() ? 0 : 1);
}

Value CommentsTable::EntryTypeAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return comment->Get("entry_type");
}

Value CommentsTable::ExpiresAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return comment->Get("expires");
}

Value CommentsTable::ExpireTimeAccessor(const Value& row)
{
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	if (!comment)
		return Empty;

	return static_cast<int>(comment->Get("expire_time"));
}
