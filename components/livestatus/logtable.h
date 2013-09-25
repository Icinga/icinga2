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

	static Value TimeAccessor(const Value& row);
	static Value LinenoAccessor(const Value& row);
	static Value ClassAccessor(const Value& row);
	static Value MessageAccessor(const Value& row);
	static Value TypeAccessor(const Value& row);
	static Value OptionsAccessor(const Value& row);
	static Value CommentAccessor(const Value& row);
	static Value PluginOutputAccessor(const Value& row);
	static Value StateAccessor(const Value& row);
	static Value StateTypeAccessor(const Value& row);
	static Value AttemptAccessor(const Value& row);
	static Value ServiceDescriptionAccessor(const Value& row);
	static Value HostNameAccessor(const Value& row);
	static Value ContactNameAccessor(const Value& row);
	static Value CommandNameAccessor(const Value& row);
};

}

#endif /* LOGTABLE_H */
