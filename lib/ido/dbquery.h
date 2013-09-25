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

#ifndef DBQUERY_H
#define DBQUERY_H

#include "base/dictionary.h"

namespace icinga
{

enum DbQueryType
{
	DbQueryInsert = 1,
	DbQueryUpdate = 2,
	DbQueryDelete = 4
};

class DbObject;

struct DbQuery
{
	int Type;
	String Table;
	Dictionary::Ptr Fields;
	Dictionary::Ptr WhereCriteria;
	boost::shared_ptr<DbObject> Object;
	bool ConfigUpdate;
	bool StatusUpdate;

	DbQuery(void)
		: Type(0), ConfigUpdate(false), StatusUpdate(false)
	{ }
};

}

#endif /* DBQUERY_H */
