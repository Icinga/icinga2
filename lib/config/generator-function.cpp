/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/debug.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/scriptframe.hpp"
#include "base/shared.hpp"
#include "base/value.hpp"
#include "config/generator-function.hpp"
#include <exception>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

using namespace icinga;

GeneratorFunction::GeneratorFunction(const Expression::Ptr& expression)
{
	auto originFrame (ScriptFrame::GetCurrentFrame());

	std::promise<void> promise;
	auto future (promise.get_future());

	m_Thread = std::thread([this, expression, originFrame, &promise]() {
		ScriptFrame frame (false);
		frame = *originFrame;

		promise.set_value();

		frame.Depth = 0;

		for (std::unique_lock<std::mutex> lock (m_ITC.Lock);;) {
			if (!m_ITC.NextQueue.empty()) {
				break;
			}

			if (m_ITC.Shutdown) {
				return;
			}

			m_ITC.CV.wait(lock);
		}

		l_GeneratorFunction = this;

		try {
			expression->Evaluate(frame);
		} catch (ForcedUnwind) {
		} catch (...) {
			std::unique_lock<std::mutex> lock (m_ITC.Lock);
			ASSERT(!m_ITC.NextQueue.empty());

			auto next (std::move(m_ITC.NextQueue.front()));
			m_ITC.NextQueue.pop();
			lock.unlock();

			next.set_exception(std::current_exception());
		}

		std::unique_lock<std::mutex> lock (m_ITC.Lock);
		m_ITC.Shutdown = true;

		while (!m_ITC.NextQueue.empty()) {
			auto next (std::move(m_ITC.NextQueue.front()));
			m_ITC.NextQueue.pop();

			next.set_value({Empty, false});
		}
	});

	future.wait();
}

GeneratorFunction::~GeneratorFunction()
{
	{
		std::unique_lock<std::mutex> lock (m_ITC.Lock);
		m_ITC.Shutdown = true;
		m_ITC.CV.notify_all();
	}

	m_Thread.join();
}

bool GeneratorFunction::GetNext(Value& out)
{
	std::unique_lock<std::mutex> lock (m_ITC.Lock);

	if (m_ITC.Shutdown) {
		return false;
	}

	std::promise<std::pair<Value, bool>> promise;
	auto future (promise.get_future());

	m_ITC.NextQueue.emplace(std::move(promise));
	m_ITC.CV.notify_all();

	lock.unlock();

	future.wait();

	auto res (future.get());

	if (res.second) {
		out = std::move(res.first);
	}

	return res.second;
}

void GeneratorFunction::YieldItem(Value item)
{
	std::unique_lock<std::mutex> lock (m_ITC.Lock);
	ASSERT(!m_ITC.NextQueue.empty());

	auto next (std::move(m_ITC.NextQueue.front()));
	m_ITC.NextQueue.pop();
	lock.unlock();

	next.set_value({std::move(item), true});

	for (lock.lock();;) {
		if (!m_ITC.NextQueue.empty()) {
			break;
		}

		if (m_ITC.Shutdown) {
			throw ForcedUnwind();
		}

		m_ITC.CV.wait(lock);
	}
}

thread_local GeneratorFunction* icinga::l_GeneratorFunction = nullptr;
