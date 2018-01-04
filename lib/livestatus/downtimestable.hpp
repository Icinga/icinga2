/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef DOWNTIMESTABLE_H
#define DOWNTIMESTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class DowntimesTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(DowntimesTable);

	DowntimesTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const override;
	virtual String GetPrefix(void) const override;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn) override;

private:
	static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value AuthorAccessor(const Value& row);
	static Value CommentAccessor(const Value& row);
	static Value IdAccessor(const Value& row);
	static Value EntryTimeAccessor(const Value& row);
	static Value TypeAccessor(const Value& row);
	static Value IsServiceAccessor(const Value& row);
	static Value StartTimeAccessor(const Value& row);
	static Value EndTimeAccessor(const Value& row);
	static Value FixedAccessor(const Value& row);
	static Value DurationAccessor(const Value& row);
	static Value TriggeredByAccessor(const Value& row);
};

}

#endif /* DOWNTIMESTABLE_H */
