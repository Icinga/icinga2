/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/commentstable.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicestable.hpp"
#include "icinga/service.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

CommentsTable::CommentsTable()
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
	ServicesTable::AddColumns(table, "service_", [objectAccessor](const Value& row, LivestatusGroupByType, const Object::Ptr&) -> Value {
		return ServiceAccessor(row, objectAccessor);
	});
	HostsTable::AddColumns(table, "host_", [objectAccessor](const Value& row, LivestatusGroupByType, const Object::Ptr&) -> Value {
		return HostAccessor(row, objectAccessor);
	});
}

String CommentsTable::GetName() const
{
	return "comments";
}

String CommentsTable::GetPrefix() const
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
