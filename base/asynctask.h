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
	static void FinishForwarder(const shared_ptr<T>& task)
	{
		task->OnTaskCompleted(task);
	}
};

}

#endif /* ASYNCTASK_H */
