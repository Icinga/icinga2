/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef COMMITCONTEXT_H
#define COMMITCONTEXT_H

#include "base/debug.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/scriptframe.hpp"
#include "config/expression.hpp"
#include <exception>
#include <memory>
#include <utility>
#include <vector>

namespace icinga
{

class CommitContext
{
public:
	static inline
	CommitContext* GetCurrent()
	{
		return m_Current;
	}

	inline CommitContext() : m_Started(false)
	{
		VERIFY(!m_Current);

		m_Current = this;
	}

	CommitContext(const CommitContext&) = delete;
	CommitContext(CommitContext&&) = delete;
	CommitContext& operator=(const CommitContext&) = delete;
	CommitContext& operator=(CommitContext&&) = delete;

	inline ~CommitContext()
	{
		VERIFY(m_Current);

		m_Current = nullptr;
	}

	inline void RegisterOnConfigCommitted(std::shared_ptr<Expression> expr)
	{
		m_OnConfigCommitted.emplace_back(std::move(expr));
	}

	inline bool RunOnConfigCommitted()
	{
		m_Started = true;

		for (auto& expr : m_OnConfigCommitted) {
			if (!expr)
				return false;

			try {
				ScriptFrame frame(false);
				expr->Evaluate(frame);
			} catch (const std::exception& ex) {
				Log(LogCritical, "config", DiagnosticInformation(ex));
				return false;
			}
		}

		return true;
	}

	inline bool HasStarted()
	{
		return m_Started;
	}

private:
	static thread_local CommitContext* m_Current;

	std::vector<std::shared_ptr<Expression>> m_OnConfigCommitted;
	bool m_Started;
};

}

#endif /* COMMITCONTEXT_H */
