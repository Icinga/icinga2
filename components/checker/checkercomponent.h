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

#ifndef CHECKERCOMPONENT_H
#define CHECKERCOMPONENT_H

namespace icinga
{

/**
 * @ingroup checker
 */
struct ServiceNextCheckExtractor
{
	typedef double result_type;

	/**
	 * @threadsafety Always.
	 */
	double operator()(const Service::Ptr& service)
	{
		double next = service->GetNextCheck();

		while (next == 0) {
			service->UpdateNextCheck();
			next = service->GetNextCheck();
		}

		return next;
	}
};

/**
 * @ingroup checker
 */
class CheckerComponent : public DynamicObject
{
public:
	typedef shared_ptr<CheckerComponent> Ptr;
	typedef weak_ptr<CheckerComponent> WeakPtr;

	typedef multi_index_container<
		Service::Ptr,
		indexed_by<
			ordered_unique<identity<Service::Ptr> >,
			ordered_non_unique<ServiceNextCheckExtractor>
		>
	> ServiceSet;

	CheckerComponent(const Dictionary::Ptr& serializedUpdate);

	virtual void Start(void);
	virtual void Stop(void);

private:
	Endpoint::Ptr m_Endpoint;

	boost::mutex m_Mutex;
	boost::condition_variable m_CV;
	bool m_Stopped;
	thread m_Thread;

	ServiceSet m_IdleServices;
	ServiceSet m_PendingServices;

	Timer::Ptr m_ResultTimer;

	void CheckThreadProc(void);
	void ResultTimerHandler(void);

	void CheckCompletedHandler(const Service::Ptr& service);

	void AdjustCheckTimer(void);

	void CheckerChangedHandler(const Service::Ptr& service);
	void NextCheckChangedHandler(const Service::Ptr& service);

	void RescheduleCheckTimer(void);
};

}

#endif /* CHECKERCOMPONENT_H */
