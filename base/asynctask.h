#ifndef ASYNCTASK_H
#define ASYNCTASK_H 

namespace icinga
{

template<typename T>
class AsyncTask : public Object
{
public:
	typedef shared_ptr<AsyncTask<T> > Ptr;
	typedef weak_ptr<AsyncTask<T> > WeakPtr;

	AsyncTask(void)
		: m_Finished(false)
	{ }

	~AsyncTask(void)
	{
		assert(m_Finished);
	}

	void Start(void)
	{
		assert(Application::IsMainThread());

		Run();
	}

	boost::signal<void (const shared_ptr<T>&)> OnTaskCompleted;

protected:
	virtual void Run(void) = 0;

	void Finish(void)
	{
		Event::Ptr ev = boost::make_shared<Event>();
		ev->OnEventDelivered.connect(boost::bind(&T::FinishForwarder, static_cast<shared_ptr<T> >(GetSelf())));
		Event::Post(ev);
	}

	bool m_Finished;

private:
	static void FinishForwarder(typename const shared_ptr<T>& task)
	{
		task->OnTaskCompleted(task);
	}
};

}

#endif /* ASYNCTASK_H */
