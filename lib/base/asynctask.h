/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef ASYNCTASK_H
#define ASYNCTASK_H

namespace icinga
{

/**
 * An asynchronous task.
 *
 * @ingroup base
 */
 template<typename TClass, typename TResult>
class AsyncTask : public Object
{
public:
	typedef shared_ptr<AsyncTask<TClass, TResult> > Ptr;
	typedef weak_ptr<AsyncTask<TClass, TResult> > WeakPtr;

	/**
	 * A completion callback for an AsyncTask.
	 */
	typedef function<void (const shared_ptr<TClass>&)> CompletionCallback;

	/**
	 * Constructor for the AsyncTask class.
	 */
	AsyncTask(void)
		: m_Finished(false), m_ResultRetrieved(false)
	{ }

	/**
	 * Destructor for the AsyncTask class.
	 */
	~AsyncTask(void)
	{
		if (!m_Finished)
			ASSERT(!"Contract violation: AsyncTask was destroyed before its completion callback was invoked.");
		else if (!m_ResultRetrieved)
			ASSERT(!"Contract violation: AsyncTask was destroyed before its result was retrieved.");
	}


	/**
	 * Starts the async task. The caller must hold a reference to the AsyncTask
	 * object until the completion callback is invoked.
	 *
	 * @threadsafety Always.
	 */
	void Start(const CompletionCallback& completionCallback = CompletionCallback())
	{
		ASSERT(!OwnsLock());
		boost::mutex::scoped_lock lock(m_Mutex);

		m_CompletionCallback = completionCallback;
		Utility::QueueAsyncCallback(boost::bind(&AsyncTask<TClass, TResult>::RunInternal, this));
	}

	/**
	 * Checks whether the task is finished.
	 *
	 * @threadsafety Always.
	 */
	bool IsFinished(void) const
	{
		ASSERT(!OwnsLock());
		boost::mutex::scoped_lock lock(m_Mutex);
		return m_Finished;
	}

	/**
	 * Blocks until the task is completed and retrieves the result. Throws an exception if one is stored in
	 * the AsyncTask object.
	 *
	 * @returns The task's result.
	 * @threadsafety Always.
	 */
	TResult GetResult(void)
	{
		ASSERT(!OwnsLock());
		boost::mutex::scoped_lock lock(m_Mutex);

		while (!m_Finished)
			m_CV.wait(lock);

		if (m_ResultRetrieved)
			BOOST_THROW_EXCEPTION(runtime_error("GetResult called on an AsyncTask whose result was already retrieved."));

		m_ResultRetrieved = true;

		if (m_Exception)
			rethrow_exception(m_Exception);

		TResult result;
		std::swap(m_Result, result);
		return result;
	}

	/**
	 * Finishes the task using an exception.
	 *
	 * @param ex The exception.
	 * @threadsafety Always.
	 */
	void FinishException(const boost::exception_ptr& ex)
	{
		ASSERT(!OwnsLock());
		boost::mutex::scoped_lock lock(m_Mutex);

		m_Exception = ex;
		FinishInternal();
	}

	/**
	 * Finishes the task using an ordinary result.
	 *
	 * @param result The result.
	 * @threadsafety Always.
	 */
	void FinishResult(const TResult& result)
	{
		ASSERT(!OwnsLock());
		boost::mutex::scoped_lock lock(m_Mutex);

		m_Result = result;
		FinishInternal();
	}

protected:
	/**
	 * Begins executing the task. The Run method must ensure
	 * that one of the Finish*() functions is executed on the task
	 * object (possibly after the Run method has returned).
	 */
	virtual void Run(void) = 0;

private:
	/**
	 * Finishes the task and causes the completion callback to be invoked. This
	 * function must be called before the object is destroyed.
	 *
	 * @threadsafety Caller must hold m_Mutex.
	 */
	void FinishInternal(void)
	{
		ASSERT(!m_Finished);
		m_Finished = true;
		m_CV.notify_all();


		if (!m_CompletionCallback.empty()) {
			CompletionCallback callback;
			m_CompletionCallback.swap(callback);

			Utility::QueueAsyncCallback(boost::bind(callback, GetSelf()));
		}
	}

	/**
	 * Calls the Run() method and catches exceptions.
	 */
	void RunInternal(void)
	{
		try {
			Run();
		} catch (const exception& ex) {
			FinishException(boost::current_exception());
		}
	}

	mutable boost::mutex m_Mutex;
	boost::condition_variable m_CV;
	CompletionCallback m_CompletionCallback; /**< The completion callback. */
	TResult m_Result; /**< The task's result. */
	boost::exception_ptr m_Exception; /**< The task's exception. */

	bool m_Finished; /**< Whether the task is finished. */
	bool m_ResultRetrieved; /**< Whether the result was retrieved. */
};

}

#endif /* ASYNCTASK_H */
