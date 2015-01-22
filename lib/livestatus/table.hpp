/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include <vector>

namespace icinga
{

typedef boost::function<void (const Value&)> AddRowFunction;

class Filter;

/**
 * @ingroup livestatus
 */
class Table : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Table);

	static Table::Ptr GetByName(const String& name, const String& compat_log_path = "", const unsigned long& from = 0, const unsigned long& until = 0);

	virtual String GetName(void) const = 0;
	virtual String GetPrefix(void) const = 0;

	std::vector<Value> FilterRows(const intrusive_ptr<Filter>& filter);

	void AddColumn(const String& name, const Column& column);
	Column GetColumn(const String& name) const;
	std::vector<String> GetColumnNames(void) const;

protected:
	Table(void);

	virtual void FetchRows(const AddRowFunction& addRowFn) = 0;

	static Value ZeroAccessor(const Value&);
	static Value OneAccessor(const Value&);
	static Value EmptyStringAccessor(const Value&);
	static Value EmptyArrayAccessor(const Value&);
	static Value EmptyDictionaryAccessor(const Value&);

private:
	std::map<String, Column> m_Columns;

	void FilteredAddRow(std::vector<Value>& rs, const intrusive_ptr<Filter>& filter, const Value& row);
};

}

#endif /* TABLE_H */

#include "livestatus/filter.hpp"
