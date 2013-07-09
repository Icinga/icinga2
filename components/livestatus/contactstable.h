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

#ifndef CONTACTSTABLE_H
#define CONTACTSTABLE_H

#include "livestatus/table.h"

using namespace icinga;

namespace livestatus
{

/**
 * @ingroup livestatus
 */
class ContactsTable : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(ContactsTable);

	ContactsTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn);

	static Value NameAccessor(const Object::Ptr& object);
	static Value AliasAccessor(const Object::Ptr& object);
	static Value EmailAccessor(const Object::Ptr& object);
	static Value PagerAccessor(const Object::Ptr& object);
	static Value HostNotificationPeriodAccessor(const Object::Ptr& object);
	static Value ServiceNotificationPeriodAccessor(const Object::Ptr& object);
	static Value CanSubmitCommandsAccessor(const Object::Ptr& object);
	static Value HostNotificationsEnabledAccessor(const Object::Ptr& object);
	static Value ServiceNotificationsEnabledAccessor(const Object::Ptr& object);
	static Value InHostNotificationPeriodAccessor(const Object::Ptr& object);
	static Value InServiceNotificationPeriodAccessor(const Object::Ptr& object);
	static Value CustomVariableNamesAccessor(const Object::Ptr& object);
	static Value CustomVariableValuesAccessor(const Object::Ptr& object);
	static Value CustomVariablesAccessor(const Object::Ptr& object);
	static Value ModifiedAttributesAccessor(const Object::Ptr& object);
	static Value ModifiedAttributesListAccessor(const Object::Ptr& object);
};

}

#endif /* CONTACTSTABLE_H */
