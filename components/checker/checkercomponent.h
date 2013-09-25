/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/service.h"
#include "base/dynamicobject.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>

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
		if (!service->HasAuthority("checker"))
			return Utility::GetTime() + 60;

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
	DECLARE_PTR_TYPEDEFS(CheckerComponent);

	typedef boost::multi_index_container<
		Service::Ptr,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<boost::multi_index::identity<Service::Ptr> >,
			boost::multi_index::ordered_non_unique<ServiceNextCheckExtractor>
		>
	> ServiceSet;

	virtual void Start(void);
	virtual void Stop(void);

private:
	boost::mutex m_Mutex;
	boost::condition_variable m_CV;
	bool m_Stopped;
	boost::thread m_Thread;

	ServiceSet m_IdleServices;
	ServiceSet m_PendingServices;

	Timer::Ptr m_ResultTimer;

	void CheckThreadProc(void);
	void ResultTimerHandler(void);

	void ExecuteCheckHelper(const Service::Ptr& service);

	void AdjustCheckTimer(void);

	void ObjectHandler(const DynamicObject::Ptr& object);
	void NextCheckChangedHandler(const Service::Ptr& service);

	void RescheduleCheckTimer(void);
};

}

#endif /* CHECKERCOMPONENT_H */
