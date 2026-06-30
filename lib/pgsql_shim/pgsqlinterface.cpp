// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pgsql_shim/pgsqlinterface.hpp"

using namespace icinga;

struct PgsqlInterfaceImpl final : public PgsqlInterface
{
	void Destroy() override
	{
		delete this;
	}

	void clear(PGresult *res) const override
	{
		PQclear(res);
	}

	char *cmdTuples(PGresult *res) const override
	{
		return PQcmdTuples(res);
	}

	char *errorMessage(const PGconn *conn) const override
	{
		return PQerrorMessage(conn);
	}

	size_t escapeStringConn(PGconn *conn, char *to, const char *from, size_t length, int *error) const override
	{
		return PQescapeStringConn(conn, to, from, length, error);
	}

	PGresult *exec(PGconn *conn, const char *query) const override
	{
		return PQexec(conn, query);
	}

	void finish(PGconn *conn) const override
	{
		PQfinish(conn);
	}

	char *fname(const PGresult *res, int field_num) const override
	{
		return PQfname(res, field_num);
	}

	int getisnull(const PGresult *res, int tup_num, int field_num) const override
	{
		return PQgetisnull(res, tup_num, field_num);
	}

	char *getvalue(const PGresult *res, int tup_num, int field_num) const override
	{
		return PQgetvalue(res, tup_num, field_num);
	}

	int isthreadsafe() const override
	{
		return PQisthreadsafe();
	}

	int nfields(const PGresult *res) const override
	{
		return PQnfields(res);
	}

	int ntuples(const PGresult *res) const override
	{
		return PQntuples(res);
	}

	char *resultErrorMessage(const PGresult *res) const override
	{
		return PQresultErrorMessage(res);
	}

	ExecStatusType resultStatus(const PGresult *res) const override
	{
		return PQresultStatus(res);
	}

	int serverVersion(const PGconn *conn) const override
	{
		return PQserverVersion(conn);
	}

	PGconn *setdbLogin(const char *pghost, const char *pgport, const char *pgoptions, const char *pgtty, const char *dbName, const char *login, const char *pwd) const override
	{
		return PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbName, login, pwd);
	}

	PGconn *connectdb(const char *conninfo) const override
	{
		return PQconnectdb(conninfo);
	}

	ConnStatusType status(const PGconn *conn) const override
	{
		return PQstatus(conn);
	}
};

PgsqlInterface *create_pgsql_shim()
{
	return new PgsqlInterfaceImpl();
}
