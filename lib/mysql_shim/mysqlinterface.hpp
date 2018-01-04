/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef MYSQLINTERFACE_H
#define MYSQLINTERFACE_H

#include "mysql_shim/mysql_shim_export.h"
#include <memory>
#include <mysql.h>

namespace icinga
{

struct MysqlInterface
{
	MysqlInterface(const MysqlInterface&) = delete;
	MysqlInterface& operator=(MysqlInterface&) = delete;

	virtual void Destroy() = 0;

	virtual my_ulonglong affected_rows(MYSQL *mysql) const = 0;
	virtual void close(MYSQL *sock) const = 0;
	virtual const char *error(MYSQL *mysql) const = 0;
	virtual MYSQL_FIELD *fetch_field(MYSQL_RES *result) const = 0;
	virtual unsigned long *fetch_lengths(MYSQL_RES *result) const = 0;
	virtual MYSQL_ROW fetch_row(MYSQL_RES *result) const = 0;
	virtual unsigned int field_count(MYSQL *mysql) const = 0;
	virtual MYSQL_FIELD_OFFSET field_seek(MYSQL_RES *result,
		MYSQL_FIELD_OFFSET offset) const = 0;
	virtual void free_result(MYSQL_RES *result) const = 0;
	virtual MYSQL *init(MYSQL *mysql) const = 0;
	virtual my_ulonglong insert_id(MYSQL *mysql) const = 0;
	virtual int next_result(MYSQL *mysql) const = 0;
	virtual int ping(MYSQL *mysql) const = 0;
	virtual int query(MYSQL *mysql, const char *q) const = 0;
	virtual MYSQL *real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd,
		const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag) const = 0;
	virtual unsigned long real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length) const = 0;
	virtual my_bool ssl_set(MYSQL *mysql, const char *key, const char *cert, const char *ca, const char *capath, const char *cipher) const = 0;
	virtual MYSQL_RES *store_result(MYSQL *mysql) const = 0;
	virtual unsigned int thread_safe() const = 0;

protected:
	MysqlInterface() = default;
	~MysqlInterface() = default;
};

struct MysqlInterfaceDeleter
{
	void operator()(MysqlInterface *ifc) const
	{
		ifc->Destroy();
	}
};

}

extern "C"
{
	MYSQL_SHIM_EXPORT icinga::MysqlInterface *create_mysql_shim();
}

typedef icinga::MysqlInterface *(*create_mysql_shim_ptr)();

#endif /* MYSQLINTERFACE_H */
