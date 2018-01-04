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

#ifndef PGSQLINTERFACE_H
#define PGSQLINTERFACE_H

#include "pgsql_shim/pgsql_shim_export.h"
#include <memory>
#include <libpq-fe.h>

namespace icinga
{

struct PgsqlInterface
{
	PgsqlInterface(const PgsqlInterface&) = delete;
	PgsqlInterface& operator=(PgsqlInterface&) = delete;

	virtual void Destroy() = 0;

	virtual void clear(PGresult *res) const = 0;
	virtual char *cmdTuples(PGresult *res) const = 0;
	virtual char *errorMessage(const PGconn *conn) const = 0;
	virtual size_t escapeStringConn(PGconn *conn, char *to, const char *from, size_t length, int *error) const = 0;
	virtual PGresult *exec(PGconn *conn, const char *query) const = 0;
	virtual void finish(PGconn *conn) const = 0;
	virtual char *fname(const PGresult *res, int field_num) const = 0;
	virtual int getisnull(const PGresult *res, int tup_num, int field_num) const = 0;
	virtual char *getvalue(const PGresult *res, int tup_num, int field_num) const = 0;
	virtual int isthreadsafe() const = 0;
	virtual int nfields(const PGresult *res) const = 0;
	virtual int ntuples(const PGresult *res) const = 0;
	virtual char *resultErrorMessage(const PGresult *res) const = 0;
	virtual ExecStatusType resultStatus(const PGresult *res) const = 0;
	virtual int serverVersion(const PGconn *conn) const = 0;
	virtual PGconn *setdbLogin(const char *pghost, const char *pgport, const char *pgoptions, const char *pgtty, const char *dbName, const char *login, const char *pwd) const = 0;
	virtual ConnStatusType status(const PGconn *conn) const = 0;

protected:
	PgsqlInterface() = default;
	~PgsqlInterface() = default;
};

struct PgsqlInterfaceDeleter
{
	void operator()(PgsqlInterface *ifc) const
	{
		ifc->Destroy();
	}
};

}

extern "C"
{
	PGSQL_SHIM_EXPORT icinga::PgsqlInterface *create_pgsql_shim();
}

typedef icinga::PgsqlInterface *(*create_pgsql_shim_ptr)();

#endif /* PGSQLINTERFACE_H */
