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

#ifndef TABLE_H
#define TABLE_H

#include "livestatus/column.hpp"
#include "base/object.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include <vector>

namespace icinga
{

struct LivestatusRowValue {
	Value Row;
	LivestatusGroupByType GroupByType;
	Value GroupByObject;
};

typedef std::function<bool (const Value&, LivestatusGroupByType, const Object::Ptr&)> AddRowFunction;

class Filter;

/**
 * @ingroup livestatus
 */
class Table : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Table);

	static Table::Ptr GetByName(const String& name, const String& compat_log_path = "", const unsigned long& from = 0, const unsigned long& until = 0);

	virtual String GetName() const = 0;
	virtual String GetPrefix() const = 0;

	std::vector<LivestatusRowValue> FilterRows(const intrusive_ptr<Filter>& filter, int limit = -1);

	void AddColumn(const String& name, const Column& column);
	Column GetColumn(const String& name) const;
	std::vector<String> GetColumnNames() const;

	LivestatusGroupByType GetGroupByType() const;

protected:
	Table(LivestatusGroupByType type = LivestatusGroupByNone);

	virtual void FetchRows(const AddRowFunction& addRowFn) = 0;

	static Value ZeroAccessor(const Value&);
	static Value OneAccessor(const Value&);
	static Value EmptyStringAccessor(const Value&);
	static Value EmptyArrayAccessor(const Value&);
	static Value EmptyDictionaryAccessor(const Value&);

	LivestatusGroupByType m_GroupByType;
	Value m_GroupByObject;

private:
	std::map<String, Column> m_Columns;

	bool FilteredAddRow(std::vector<LivestatusRowValue>& rs, const intrusive_ptr<Filter>& filter, int limit, const Value& row, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject);
};

}

#endif /* TABLE_H */

#include "livestatus/filter.hpp"
