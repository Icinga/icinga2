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

#ifndef COMMENTSTABLE_H
#define COMMENTSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class CommentsTable final : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(CommentsTable);

	CommentsTable();

	static void AddColumns(Table *table, const String& prefix = String(),
		const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	String GetName() const override;
	String GetPrefix() const override;

protected:
	void FetchRows(const AddRowFunction& addRowFn) override;

private:
	static Object::Ptr HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);
	static Object::Ptr ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor);

	static Value AuthorAccessor(const Value& row);
	static Value CommentAccessor(const Value& row);
	static Value IdAccessor(const Value& row);
	static Value EntryTimeAccessor(const Value& row);
	static Value TypeAccessor(const Value& row);
	static Value IsServiceAccessor(const Value& row);
	static Value EntryTypeAccessor(const Value& row);
	static Value ExpiresAccessor(const Value& row);
	static Value ExpireTimeAccessor(const Value& row);
};

}

#endif /* COMMENTSTABLE_H */
