/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef HOSTGROUPSTABLE_H
#define HOSTGROUPSTABLE_H

#include "livestatus/table.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class I2_LIVESTATUS_API HostGroupsTable : public Table
{
public:
	DECLARE_PTR_TYPEDEFS(HostGroupsTable);

	HostGroupsTable(void);

	static void AddColumns(Table *table, const String& prefix = String(),
	    const Column::ObjectAccessor& objectAccessor = Column::ObjectAccessor());

	virtual String GetName(void) const override;
	virtual String GetPrefix(void) const override;

protected:
	virtual void FetchRows(const AddRowFunction& addRowFn) override;

	static Value NameAccessor(const Value& row);
	static Value AliasAccessor(const Value& row);
	static Value NotesAccessor(const Value& row);
	static Value NotesUrlAccessor(const Value& row);
	static Value ActionUrlAccessor(const Value& row);
	static Value MembersAccessor(const Value& row);
	static Value MembersWithStateAccessor(const Value& row);
	static Value WorstHostStateAccessor(const Value& row);
	static Value NumHostsAccessor(const Value& row);
	static Value NumHostsPendingAccessor(const Value& row);
	static Value NumHostsUpAccessor(const Value& row);
	static Value NumHostsDownAccessor(const Value& row);
	static Value NumHostsUnreachAccessor(const Value& row);
	static Value NumServicesAccessor(const Value& row);
	static Value WorstServicesStateAccessor(const Value& row);
	static Value NumServicesPendingAccessor(const Value& row);
	static Value NumServicesOkAccessor(const Value& row);
	static Value NumServicesWarnAccessor(const Value& row);
	static Value NumServicesCritAccessor(const Value& row);
	static Value NumServicesUnknownAccessor(const Value& row);
	static Value WorstServiceHardStateAccessor(const Value& row);
	static Value NumServicesHardOkAccessor(const Value& row);
	static Value NumServicesHardWarnAccessor(const Value& row);
	static Value NumServicesHardCritAccessor(const Value& row);
	static Value NumServicesHardUnknownAccessor(const Value& row);
};

}

#endif /* HOSTGROUPSTABLE_H */
