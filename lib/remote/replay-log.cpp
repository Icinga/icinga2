/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/string.hpp"
#include "remote/apilistener.hpp"
#include "remote/replay-log.hpp"
#include <cstdio>
#include <SQLiteCpp/Database.h>
#include <utility>

using namespace icinga;

ReplayLogIterator::ReplayLogIterator(SQLite::Database& db)
	: m_Stmt(Shared<SQLite::Statement>::Make(db, "SELECT ts, content FROM message ORDER BY ts"))
{
	Next();
}

void ReplayLogIterator::Next()
{
	if (m_Stmt) {
		if (m_Stmt->executeStep()) {
			m_Message.first = m_Stmt->getColumn(0).getDouble();

			auto content (m_Stmt->getColumn(1));
			auto begin ((const char*)content.getBlob());

			m_Message.second = String(begin, begin + content.getBytes());
		} else {
			m_Stmt = nullptr;
		}
	}
}

static inline
String EndpointToFile(const String& endpoint)
{
	auto file (ApiListener::GetApiDir() + "log/");

	for (auto c : endpoint) {
		char buf[3] = { 0 };
		sprintf(buf, "%02x", (unsigned int)(((int)c + 256) % 256));

		file += (char*)buf;
	}

	file += ".sqlite3";

	return std::move(file);
}

ReplayLog::ReplayLog(const String& endpoint) : ReplayLog(endpoint, EndpointToFile(endpoint))
{
}

void ReplayLog::Log(double ts, const String& message)
{
	SQLite::Statement stmt (m_DB, "INSERT INTO message (ts, content) VALUES (?, ?)");
	stmt.bind(1, ts);
	stmt.bindNoCopy(2, message.GetData());

	(void)stmt.exec();
}

void ReplayLog::Cleanup(double upTo)
{
	SQLite::Statement stmt (m_DB, "DELETE FROM message WHERE ts < ?");
	stmt.bind(1, upTo);

	(void)stmt.exec();
}

ReplayLog::ReplayLog(const String& endpoint, const String& file)
	: m_DB(file.CStr(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	m_DB.exec("CREATE TABLE IF NOT EXISTS message ( ts REAL, content BLOB )");
	m_DB.exec("CREATE INDEX IF NOT EXISTS ix_message_ts ON message ( ts )");
}
