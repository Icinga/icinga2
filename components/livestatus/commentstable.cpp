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

#include "livestatus/commentstable.h"
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
	table->AddColumn(prefix + "persistent", Column(&CommentsTable::PersistentAccessor, objectAccessor));
	table->AddColumn(prefix + "source", Column(&CommentsTable::SourceAccessor, objectAccessor));
	table->AddColumn(prefix + "entry_type", Column(&CommentsTable::EntryTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "expires", Column(&CommentsTable::ExpiresAccessor, objectAccessor));
	table->AddColumn(prefix + "expire_time", Column(&CommentsTable::ExpireTimeAccessor, objectAccessor));

	// TODO: Join services table.
}

String CommentsTable::GetName(void) const
{
	return "comments";
}

void CommentsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);
		Dictionary::Ptr comments = service->GetComments();

		if (!comments)
			continue;

		ObjectLock olock(comments);

		Value comment;
		BOOST_FOREACH(boost::tie(boost::tuples::ignore, comment), comments) {
			addRowFn(comment);
		}
	}
}

Value CommentsTable::AuthorAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("author");
	*/
}

Value CommentsTable::CommentAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("text");
	*/
}

Value CommentsTable::IdAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("legacy_id");
	*/
}

Value CommentsTable::EntryTimeAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("entry_time");
	*/
}

Value CommentsTable::TypeAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value CommentsTable::IsServiceAccessor(const Object::Ptr& object)
{
	/*
	Service::Ptr svc = Service::GetOwnerByCommentID(row);

	return (svc->IsHostCheck() ? 0 : 1);
	*/
}

Value CommentsTable::PersistentAccessor(const Object::Ptr& object)
{
	/* TODO - always 1 */
	return 1;
}

Value CommentsTable::SourceAccessor(const Object::Ptr& object)
{
	/* TODO */
	return Value();
}

Value CommentsTable::EntryTypeAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("entry_type");
	*/
}

Value CommentsTable::ExpiresAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("expires");
	*/
}

Value CommentsTable::ExpireTimeAccessor(const Object::Ptr& object)
{
	/*
	Dictionary::Ptr comment = Service::GetCommentByID(row);

	return comment->Get("expire_time");
	*/
}
