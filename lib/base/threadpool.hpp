/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "base/atomic.hpp"
#include "base/configuration.hpp"
#include "base/exception.hpp"
#include "base/logger.hpp"
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cstdint>

namespace icinga
{

enum SchedulerPolicy
{
	DefaultScheduler,
	LowLatencyScheduler
};

/**
 * A thread pool.
 *
 * @ingroup base
 */
class ThreadPool
{
public:
	typedef std::function<void ()> WorkFunction;

	ThreadPool();
	~ThreadPool();

	void Start();
	void Stop();

	/**
	 * Appends a work item to the work queue. Work items will be processed in FIFO order.
	 *
	 * @param callback The callback function for the work item.
	 * @returns true if the item was queued, false otherwise.
	 */
	template<class T>
	bool Post(T callback, SchedulerPolicy)
	{
		boost::shared_lock<decltype(m_Mutex)> lock (m_Mutex);

		if (m_Pool) {
			m_Pending.fetch_add(1);

			boost::asio::post(*m_Pool, [this, callback]() {
				m_Pending.fetch_sub(1);

				try {
					callback();
				} catch (const std::exception& ex) {
					Log(LogCritical, "ThreadPool")
						<< "Exception thrown in event handler:\n"
						<< DiagnosticInformation(ex);
				} catch (...) {
					Log(LogCritical, "ThreadPool", "Exception of unknown type thrown in event handler.");
				}
			});

			return true;
		} else {
			return false;
		}
	}

	/**
	 * Returns the amount of queued tasks not started yet.
	 *
	 * @returns amount of queued tasks.
	 */
	inline uint_fast64_t GetPending()
	{
		return m_Pending.load();
	}

private:
	boost::shared_mutex m_Mutex;
	std::unique_ptr<boost::asio::thread_pool> m_Pool;
	Atomic<uint_fast64_t> m_Pending;
};

}

#endif /* THREADPOOL_H */
