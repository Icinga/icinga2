/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKERCOMPONENT_H
#define CHECKERCOMPONENT_H

#include "checker/checkercomponent-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/wait-group.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace icinga
{

/**
 * @ingroup checker
 */
struct CheckableScheduleInfo
{
	Checkable::Ptr Object;
	double NextCheck;

	/**
	 * Get the index value for ordering in the multi-index container.
	 *
	 * This function returns a very large value for checkables that have a running check, effectively pushing
	 * them to the end of the ordering. This ensures that checkables with running checks are not prioritized
	 * for scheduling ahead of others. Rescheduling of such checkables is unnecessary because the checkable
	 * is going to reject this anyway if it notices that a check is already running, so avoiding unnecessary
	 * CPU load. Once the running check is finished, the checkable will be re-inserted into the set with its
	 * actual next check time as the index value.
	 *
	 * @return The index value for ordering in the multi-index container.
	 */
	double Index() const
	{
		if (Object->HasRunningCheck()) {
			return std::numeric_limits<double>::max();
		}
		return NextCheck;
	}
};

/**
 * @ingroup checker
 */
class CheckerComponent final : public ObjectImpl<CheckerComponent>
{
public:
	DECLARE_OBJECT(CheckerComponent);
	DECLARE_OBJECTNAME(CheckerComponent);

	typedef boost::multi_index_container<
		CheckableScheduleInfo,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<boost::multi_index::member<CheckableScheduleInfo, Checkable::Ptr, &CheckableScheduleInfo::Object> >,
			boost::multi_index::ordered_non_unique<boost::multi_index::const_mem_fun<CheckableScheduleInfo, double, &CheckableScheduleInfo::Index>>
		>
	> CheckableSet;

	void OnConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	unsigned long GetIdleCheckables();
	unsigned long GetPendingCheckables();

	void SetResultTimerInterval(double interval);

private:
	std::mutex m_Mutex;
	std::condition_variable m_CV;
	bool m_Stopped{false};
	std::thread m_Thread;

	double m_ResultTimerInterval{5.0}; // Interval in seconds for the result timer.

	CheckableSet m_IdleCheckables;
	CheckableSet m_PendingCheckables;

	StoppableWaitGroup::Ptr m_WaitGroup = new StoppableWaitGroup();
	Timer::Ptr m_ResultTimer;

	void CheckThreadProc();
	void ResultTimerHandler();

	void ExecuteCheckHelper(const Checkable::Ptr& checkable);

	void ObjectHandler(const ConfigObject::Ptr& object);
	void NextCheckChangedHandler(const Checkable::Ptr& checkable, double nextCheck = -1);

	static CheckableScheduleInfo GetCheckableScheduleInfo(const Checkable::Ptr& checkable);
};

}

#endif /* CHECKERCOMPONENT_H */
