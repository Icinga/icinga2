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

#include "mysqldbconnection.h"

using namespace icinga;

REGISTER_TYPE(MysqlDbConnection);

MysqlDbConnection::MysqlDbConnection(const Dictionary::Ptr& serializedUpdate)
	: DbConnection(serializedUpdate)
{
	RegisterAttribute("host", Attribute_Config, &m_Host);
	RegisterAttribute("port", Attribute_Config, &m_Port);
	RegisterAttribute("user", Attribute_Config, &m_User);
	RegisterAttribute("password", Attribute_Config, &m_Password);
	RegisterAttribute("database", Attribute_Config, &m_Database);
}

void MysqlDbConnection::UpdateObject(const DbObject::Ptr& dbobj, DbUpdateType kind) {

}