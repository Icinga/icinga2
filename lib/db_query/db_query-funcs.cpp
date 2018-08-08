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

#include "mysql_shim/mysqlinterface.hpp"
#include "base/utility.hpp"
#include "base/function.hpp"
#include "base/library.hpp"
#include "base/logger.hpp"

using namespace icinga;

struct ConnectionInfo
{
	Library ShimLibrary;
	std::unique_ptr<MysqlInterface, MysqlInterfaceDeleter> Funcs;
	MYSQL Connection;
	bool needsClose{false};

	~ConnectionInfo()
	{
		if (needsClose)
			Funcs->close(&Connection);
	}
};

static Dictionary::Ptr fetchRow(const std::shared_ptr<ConnectionInfo>& ci, MYSQL_RES *result)
{
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	unsigned long *lengths, i;

	row = ci->Funcs->fetch_row(result);

	if (!row)
		return nullptr;

	lengths = ci->Funcs->fetch_lengths(result);

	if (!lengths)
		return nullptr;

	Dictionary::Ptr dict = new Dictionary();

	ci->Funcs->field_seek(result, 0);
	for (field = ci->Funcs->fetch_field(result), i = 0; field; field = ci->Funcs->fetch_field(result), i++)
		dict->Set(field->name, String(row[i], row[i] + lengths[i]));

	return dict;
}

static Value DBConnect(const String& host, const String& user, const String& password, const String& database)
{
	auto ci = std::make_shared<ConnectionInfo>();

	Library shimLibrary{"mysql_shim"};
	auto create_mysql_shim = shimLibrary.GetSymbolAddress<create_mysql_shim_ptr>("create_mysql_shim");
	ci->Funcs.reset(create_mysql_shim());
	std::swap(ci->ShimLibrary, shimLibrary);

	const char *raw_host = (!host.IsEmpty()) ? host.CStr() : nullptr;
	const char *raw_user = (!user.IsEmpty()) ? user.CStr() : nullptr;
	const char *raw_password = (!password.IsEmpty()) ? password.CStr() : nullptr;
	const char *raw_database = (!database.IsEmpty()) ? database.CStr() : nullptr;

	/* Connection */
	if (!ci->Funcs->init(&ci->Connection)) {
		Log(LogCritical, "IdoMysqlConnection")
				<< "mysql_init() failed: out of memory";

		BOOST_THROW_EXCEPTION(std::bad_alloc());
	}

	ci->needsClose = true;

	if (!ci->Funcs->real_connect(&ci->Connection, raw_host, raw_user, raw_password, raw_database, 3306, nullptr, CLIENT_FOUND_ROWS | CLIENT_MULTI_STATEMENTS)) {
		Log(LogCritical, "IdoMysqlConnection")
				<< "Connection to database '" << database << "' with user '" << user << "' on '" << host << ":" << "3306"
				<< "' failed: \"" << ci->Funcs->error(&ci->Connection) << "\"";

		BOOST_THROW_EXCEPTION(std::runtime_error(ci->Funcs->error(&ci->Connection)));
	}

	auto func = [ci](const std::vector<Value>& args) {
		if (args.size() < 1)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for function"));

		String query = args[0];
		if (ci->Funcs->query(&ci->Connection, query.CStr()) == 1)
			BOOST_THROW_EXCEPTION(std::runtime_error(ci->Funcs->error(&ci->Connection)));

		MYSQL_RES *result = ci->Funcs->store_result(&ci->Connection);

		auto resultPtr = std::shared_ptr<MYSQL_RES>(result, std::bind(&MysqlInterface::free_result, std::cref(ci->Funcs), _1));

		if (!result)
			return Empty;

		Array::Ptr rows = new Array();
		while (Dictionary::Ptr row = fetchRow(ci, result))
			rows->Add(row);

		return Value(rows);
	};

	return new Function("db_query for database '" + database + "' on host '" + host + "'", func, { "query" }, true);
}

REGISTER_SCRIPTFUNCTION_NS(System, db_connect, &DBConnect, "host:user:password:database");