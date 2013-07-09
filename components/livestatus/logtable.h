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

#ifndef LOGTABLE_H
#define LOGTABLE_H

#include "livestatus/table.h"

using namespace icinga;

namespace livestatus
{

/**
 * @ingroup livestatus
 */
class LogTable : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(LogTable);

	LogTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn);

	static Value TimeAccessor(const Object::Ptr& object);
	static Value LinenoAccessor(const Object::Ptr& object);
	static Value ClassAccessor(const Object::Ptr& object);
	static Value MessageAccessor(const Object::Ptr& object);
	static Value TypeAccessor(const Object::Ptr& object);
	static Value OptionsAccessor(const Object::Ptr& object);
	static Value CommentAccessor(const Object::Ptr& object);
	static Value PluginOutputAccessor(const Object::Ptr& object);
	static Value StateAccessor(const Object::Ptr& object);
	static Value StateTypeAccessor(const Object::Ptr& object);
	static Value AttemptAccessor(const Object::Ptr& object);
	static Value ServiceDescriptionAccessor(const Object::Ptr& object);
	static Value HostNameAccessor(const Object::Ptr& object);
	static Value ContactNameAccessor(const Object::Ptr& object);
	static Value CommandNameAccessor(const Object::Ptr& object);
};

}

#endif /* LOGTABLE_H */
