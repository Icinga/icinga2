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
			assert(!"Contract violation: AsyncTask was destroyed before its completion callback was invoked.");
		else if (!m_ResultRetrieved)
			assert(!"Contract violation: AsyncTask was destroyed before its result was retrieved.");
	}


	/**
	 * Starts the async task. The caller must hold a reference to the AsyncTask
	 * object until the completion callback is invoked.
	 */
	void Start(const CompletionCallback& completionCallback = CompletionCallback())
	{
		m_CompletionCallback = completionCallback;

		try {
	  		Run();
		} catch (...) {
	     		FinishException(boost::current_exception());
		}
	}

	/**
	 * Checks whether the task is finished.
	 */
	bool IsFinished(void) const
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		return m_Finished;
	}

	/**
	 * Retrieves the result of the task. Throws an exception if one is stored in
	 * the AsyncTask object.
	 *
	 * @returns The task's result.
	 */
	TResult GetResult(void)
	{
		if (!m_Finished)
			BOOST_THROW_EXCEPTION(runtime_error("GetResult called on an unfinished AsyncTask"));

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
	 */
	void FinishException(const boost::exception_ptr& ex)
	{
		m_Exception = ex;
		FinishInternal();
	}

	/**
	 * Finishes the task using an ordinary result.
	 *
	 * @param result The result.
	 */
	void FinishResult(const TResult& result)
	{
		m_Result = result;
		FinishInternal();
	}

	/**
	 * Blocks until the task is completed.
	 */
	void Wait(void)
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		while (!m_Finished)
			m_CV.wait(lock);
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
	 */
	void FinishInternal(void)
	{
		{
			boost::mutex::scoped_lock lock(m_Mutex);
			assert(!m_Finished);

			m_Finished = true;

			m_CV.notify_all();
		}

		if (!m_CompletionCallback.empty()) {
			m_CompletionCallback(GetSelf());

			/* Clear callback because the bound function might hold a
			 * reference to this task. */
			m_CompletionCallback = CompletionCallback();
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
