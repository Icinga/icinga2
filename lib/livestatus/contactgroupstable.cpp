/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/contactgroupstable.hpp"
#include "icinga/usergroup.hpp"
#include "base/configtype.hpp"

using namespace icinga;

ContactGroupsTable::ContactGroupsTable()
{
	AddColumns(this);
}

void ContactGroupsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ContactGroupsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&ContactGroupsTable::AliasAccessor, objectAccessor));
	table->AddColumn(prefix + "members", Column(&ContactGroupsTable::MembersAccessor, objectAccessor));
}

String ContactGroupsTable::GetName() const
{
	return "contactgroups";
}

String ContactGroupsTable::GetPrefix() const
{
	return "contactgroup";
}

void ContactGroupsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const auto& ug : ConfigType::GetObjectsByType<UserGroup>()) {
		if (!addRowFn(ug, LivestatusGroupByNone, Empty))
			return;
	}
}

Value ContactGroupsTable::NameAccessor(const Value& row)
{
	UserGroup::Ptr user_group = static_cast<UserGroup::Ptr>(row);

	if (!user_group)
		return Empty;

	return user_group->GetName();
}

Value ContactGroupsTable::AliasAccessor(const Value& row)
{
	UserGroup::Ptr user_group = static_cast<UserGroup::Ptr>(row);

	if (!user_group)
		return Empty;

	return user_group->GetName();
}

Value ContactGroupsTable::MembersAccessor(const Value& row)
{
	UserGroup::Ptr user_group = static_cast<UserGroup::Ptr>(row);

	if (!user_group)
		return Empty;

	ArrayData result;

	for (const auto& user : user_group->GetMembers()) {
		result.push_back(user->GetName());
	}

	return new Array(std::move(result));
}
