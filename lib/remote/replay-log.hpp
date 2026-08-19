/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef REPLAY_LOG_H
#define REPLAY_LOG_H

#include "base/shared.hpp"
#include "base/string.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

namespace icinga
{

/**
 * An endpoint's cluster messages iterator.
 *
 * @ingroup remote
 */
class ReplayLogIterator
{
public:
	ReplayLogIterator() = default;
	ReplayLogIterator(SQLite::Database& db);

	ReplayLogIterator(const ReplayLogIterator&) = delete;
	ReplayLogIterator(ReplayLogIterator&&) = default;
	ReplayLogIterator& operator=(const ReplayLogIterator&) = delete;
	ReplayLogIterator& operator=(ReplayLogIterator&&) = default;

	inline bool operator!=(const ReplayLogIterator& rhs)
	{
		return m_Stmt != rhs.m_Stmt;
	}

	inline ReplayLogIterator& operator++()
	{
		Next();
		return *this;
	}

	inline const std::pair<double, String>& operator*()
	{
		return m_Message;
	}

private:
	void Next();

	Shared<SQLite::Statement>::Ptr m_Stmt;
	std::pair<double, String> m_Message;
};

/**
 * An endpoint's cluster messages log.
 *
 * @ingroup remote
 */
class ReplayLog
{
public:
	ReplayLog(const String& endpoint);
	ReplayLog(const ReplayLog&) = delete;
	ReplayLog(ReplayLog&&) = default;
	ReplayLog& operator=(const ReplayLog&) = delete;
	ReplayLog& operator=(ReplayLog&&) = default;

	inline ReplayLogIterator begin()
	{
		return ReplayLogIterator(m_DB);
	}

	inline ReplayLogIterator end()
	{
		return ReplayLogIterator();
	}

	void Log(double ts, const String& message);
	void Cleanup(double upTo);

private:
	ReplayLog(const String& endpoint, const String& file);

	SQLite::Database m_DB;
};

}

#endif /* REPLAY_LOG_H */
