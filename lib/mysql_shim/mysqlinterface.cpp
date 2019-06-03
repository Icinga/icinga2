/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "mysql_shim/mysqlinterface.hpp"

using namespace icinga;

struct MysqlInterfaceImpl final : public MysqlInterface
{
	void Destroy() override
	{
		delete this;
	}

	my_ulonglong affected_rows(MYSQL *mysql) const override
	{
		return mysql_affected_rows(mysql);
	}

	void close(MYSQL *sock) const override
	{
		return mysql_close(sock);
	}

	const char *error(MYSQL *mysql) const override
	{
		return mysql_error(mysql);
	}

	MYSQL_FIELD *fetch_field(MYSQL_RES *result) const override
	{
		return mysql_fetch_field(result);
	}

	unsigned long *fetch_lengths(MYSQL_RES *result) const override
	{
		return mysql_fetch_lengths(result);
	}

	MYSQL_ROW fetch_row(MYSQL_RES *result) const override
	{
		return mysql_fetch_row(result);
	}

	unsigned int field_count(MYSQL *mysql) const override
	{
		return mysql_field_count(mysql);
	}

	MYSQL_FIELD_OFFSET field_seek(MYSQL_RES *result, MYSQL_FIELD_OFFSET offset) const override
	{
		return mysql_field_seek(result, offset);
	}

	void free_result(MYSQL_RES *result) const override
	{
		mysql_free_result(result);
	}

	MYSQL *init(MYSQL *mysql) const override
	{
		return mysql_init(mysql);
	}

	my_ulonglong insert_id(MYSQL *mysql) const override
	{
		return mysql_insert_id(mysql);
	}

	int next_result(MYSQL *mysql) const override
	{
		return mysql_next_result(mysql);
	}

	int ping(MYSQL *mysql) const override
	{
		return mysql_ping(mysql);
	}

	int query(MYSQL *mysql, const char *q) const override
	{
		return mysql_query(mysql, q);
	}

	MYSQL *real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd,
		const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag) const override
	{
		return mysql_real_connect(mysql, host, user, passwd, db, port, unix_socket, clientflag);
	}

	unsigned long real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length) const override
	{
		return mysql_real_escape_string(mysql, to, from, length);
	}

	bool ssl_set(MYSQL *mysql, const char *key, const char *cert, const char *ca, const char *capath, const char *cipher) const override
	{
		return mysql_ssl_set(mysql, key, cert, ca, capath, cipher);
	}

	MYSQL_RES *store_result(MYSQL *mysql) const override
	{
		return mysql_store_result(mysql);
	}

	unsigned int thread_safe() const override
	{
		return mysql_thread_safe();
	}
};

MysqlInterface *create_mysql_shim()
{
	return new MysqlInterfaceImpl();
}
