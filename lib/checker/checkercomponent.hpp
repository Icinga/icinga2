/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "checker/checkercomponent.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
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
struct CheckableNextCheckExtractor
{
	typedef double result_type;

	/**
	 * @threadsafety Always.
	 */
	double operator()(const Checkable::Ptr& checkable)
	{
		return checkable->GetNextCheck();
	}
};

/**
 * @ingroup checker
 */
class CheckerComponent : public ObjectImpl<CheckerComponent>
{
public:
	DECLARE_OBJECT(CheckerComponent);
	DECLARE_OBJECTNAME(CheckerComponent);

	typedef boost::multi_index_container<
		Checkable::Ptr,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<boost::multi_index::identity<Checkable::Ptr> >,
			boost::multi_index::ordered_non_unique<CheckableNextCheckExtractor>
		>
	> CheckableSet;

	CheckerComponent(void);

	virtual void OnConfigLoaded(void) override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	unsigned long GetIdleCheckables(void);
	unsigned long GetPendingCheckables(void);

private:
	boost::mutex m_Mutex;
	boost::condition_variable m_CV;
	bool m_Stopped;
	boost::thread m_Thread;

	CheckableSet m_IdleCheckables;
	CheckableSet m_PendingCheckables;

	Timer::Ptr m_ResultTimer;

	void CheckThreadProc(void);
	void ResultTimerHandler(void);

	void ExecuteCheckHelper(const Checkable::Ptr& checkable);

	void AdjustCheckTimer(void);

	void ObjectHandler(const ConfigObject::Ptr& object);
	void NextCheckChangedHandler(const Checkable::Ptr& checkable);

	void RescheduleCheckTimer(void);
};

}

#endif /* CHECKERCOMPONENT_H */
