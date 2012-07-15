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

class I2_BASE_API AsyncTaskBase : public Object
{
public:
	typedef shared_ptr<AsyncTaskBase> Ptr;
	typedef weak_ptr<AsyncTaskBase> WeakPtr;

	AsyncTaskBase(void);
	~AsyncTaskBase(void);

	void Start(void);
	void Finish(const boost::exception_ptr& ex);

	bool CallWithExceptionGuard(function<void ()> function);

protected:
	virtual void Run(void) = 0;

	virtual void InvokeCompletionCallback(void) = 0;

	void FinishInternal(void);

	bool m_Finished; /**< Whether the task is finished. */
	bool m_ResultRetrieved; /**< Whether the result was retrieved. */
	boost::exception_ptr m_Exception;
};

/**
 * An asynchronous task.
 *
 * @ingroup base
 */
 template<typename TClass, typename TResult>
class AsyncTask : public AsyncTaskBase
{
public:
	typedef shared_ptr<AsyncTask<TClass, TResult> > Ptr;
	typedef weak_ptr<AsyncTask<TClass, TResult> > WeakPtr;

	typedef function<void (const typename shared_ptr<TClass>&)> CompletionCallback;

	/**
	 * Constructor for the AsyncTask class.
	 *
	 * @param completionCallback Function that is called when the task is completed.
	 */
	AsyncTask(const CompletionCallback& completionCallback)
		: m_CompletionCallback(completionCallback)
	{ }

	/**
	 * Retrieves the result of the task. Throws an exception if one is stored in
	 * the AsyncTask object.
	 *
	 * @returns The task's result.
	 */
	TResult GetResult(void)
	{
		if (!m_Finished)
			throw runtime_error("GetResult called on an unfinished AsyncTask");

		if (m_ResultRetrieved)
			throw runtime_error("GetResult called on an AsyncTask whose result was already retrieved.");

		if (m_Exception)
			boost::rethrow_exception(m_Exception);

		return m_Result;
	}

	void Finish(const TResult& result)
	{
		m_Result = result;
		FinishInternal();
	}

private:
	/**
	 * Used by the Finish method to proxy the completion callback into the main
	 * thread. Invokes the completion callback and marks the task as finished.
	 */
	virtual void InvokeCompletionCallback(void)
	{
		m_Finished = true;
		m_CompletionCallback(GetSelf());

		/* Clear callback because the bound function might hold a
		 * reference to this task. */
		m_CompletionCallback = CompletionCallback();
	}

	CompletionCallback m_CompletionCallback; /**< The completion callback. */
	TResult m_Result; /**< The task's result. */
};

}

#endif /* ASYNCTASK_H */
