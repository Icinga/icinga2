/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "livestatus/commentstable.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicestable.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include <boost/tuple/tuple.hpp>

using namespace icinga;

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

	/* order is important - host w/o services must not be empty */
	ServicesTable::AddColumns(table, "service_", std::bind(&CommentsTable::ServiceAccessor, _1, objectAccessor));
	HostsTable::AddColumns(table, "host_", std::bind(&CommentsTable::HostAccessor, _1, objectAccessor));
}

String CommentsTable::GetName(void) const
{
	return "comments";
}

String CommentsTable::GetPrefix(void) const
{
	return "comment";
}

void CommentsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const Comment::Ptr& comment : ConfigType::GetObjectsByType<Comment>()) {
		if (!addRowFn(comment, LivestatusGroupByNone, Empty))
			return;
	}
}

Object::Ptr CommentsTable::HostAccessor(const Value& row, const Column::ObjectAccessor&)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	Checkable::Ptr checkable = comment->GetCheckable();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	return host;
}

Object::Ptr CommentsTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor&)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	Checkable::Ptr checkable = comment->GetCheckable();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	return service;
}

Value CommentsTable::AuthorAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return comment->GetAuthor();
}

Value CommentsTable::CommentAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return comment->GetText();
}

Value CommentsTable::IdAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return comment->GetLegacyId();
}

Value CommentsTable::EntryTimeAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return static_cast<int>(comment->GetEntryTime());
}

Value CommentsTable::TypeAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);
	Checkable::Ptr checkable = comment->GetCheckable();

	if (!checkable)
		return Empty;

	if (dynamic_pointer_cast<Host>(checkable))
		return 1;
	else
		return 2;
}

Value CommentsTable::IsServiceAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);
	Checkable::Ptr checkable = comment->GetCheckable();

	if (!checkable)
		return Empty;

	return (dynamic_pointer_cast<Host>(checkable) ? 0 : 1);
}

Value CommentsTable::EntryTypeAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return comment->GetEntryType();
}

Value CommentsTable::ExpiresAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return comment->GetExpireTime() != 0;
}

Value CommentsTable::ExpireTimeAccessor(const Value& row)
{
	Comment::Ptr comment = static_cast<Comment::Ptr>(row);

	if (!comment)
		return Empty;

	return static_cast<int>(comment->GetExpireTime());
}
