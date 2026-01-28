// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONTACTSTABLE_H
#define CONTACTSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class ContactsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(ContactsTable);

	ContactsTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

	static Value NameAccessor(const Value& row);
	static Value AliasAccessor(const Value& row);
	static Value EmailAccessor(const Value& row);
	static Value PagerAccessor(const Value& row);
	static Value HostNotificationPeriodAccessor(const Value& row);
	static Value ServiceNotificationPeriodAccessor(const Value& row);
	static Value HostNotificationsEnabledAccessor(const Value& row);
	static Value ServiceNotificationsEnabledAccessor(const Value& row);
	static Value InHostNotificationPeriodAccessor(const Value& row);
	static Value InServiceNotificationPeriodAccessor(const Value& row);
	static Value CustomVariableNamesAccessor(const Value& row);
	static Value CustomVariableValuesAccessor(const Value& row);
	static Value CustomVariablesAccessor(const Value& row);
	static Value CVIsJsonAccessor(const Value& row);
};

}

#endif /* CONTACTSTABLE_H */
