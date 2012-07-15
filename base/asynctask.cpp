#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the AsyncTaskBase class.
 */
AsyncTaskBase::AsyncTaskBase(void)
	: m_Finished(false), m_ResultRetrieved(false)
{ }

/**
 * Destructor for the AsyncTaskBase class.
 */
AsyncTaskBase::~AsyncTaskBase(void)
{
	if (!m_Finished) {
		Logger::Write(LogCritical, "base", "Contract violation: "
			"AsyncTask was destroyed before its completion callback was invoked.");
	} else if (!m_ResultRetrieved) {
		Logger::Write(LogCritical, "base", "Contract violation: "
			"AsyncTask was destroyed before its result was retrieved.");
	}
}

/**
 * Starts the async task. The caller must hold a reference to the AsyncTask
 * object until the completion callback is invoked.
 */
void AsyncTaskBase::Start(void)
{
	assert(Application::IsMainThread());

	CallWithExceptionGuard(boost::bind(&AsyncTaskBase::Run, this));
}

/**
 * Finishes the task using an exception.
 *
 * @param ex The exception.
 */
void AsyncTaskBase::Finish(const boost::exception_ptr& ex)
{
	m_Exception = ex;

	FinishInternal();
}

/**
 * Finishes the task and causes the completion callback to be invoked. This
 * function must be called before the object is destroyed.
 */
void AsyncTaskBase::FinishInternal(void)
{
	assert(!m_Finished);

	Event::Post(boost::bind(&AsyncTaskBase::InvokeCompletionCallback,
	    static_cast<AsyncTaskBase::Ptr>(GetSelf())));
}

/**
 * Invokes the provided callback function and catches any exceptions it throws.
 * Exceptions are stored into the task so that they can be re-thrown when the
 * task owner calls GetResult().
 *
 * @param task The task where exceptions should be saved.
 * @param function The function that should be invoked.
 * @returns true if no exception occured, false otherwise.
 */
bool AsyncTaskBase::CallWithExceptionGuard(function<void ()> function)
{
	try {
		function();

		return true;
	} catch (const exception&) {
		Finish(boost::current_exception());

		return false;
	}
}
