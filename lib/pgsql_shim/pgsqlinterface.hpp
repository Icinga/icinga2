// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	virtual PGconn *connectdb(const char *conninfo) const = 0;
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
