/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKERCOMPONENT_H
#define CHECKERCOMPONENT_H

#include "checker/checkercomponent-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
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
};

/**
 * @ingroup checker
 */
struct CheckableNextCheckExtractor
{
	typedef double result_type;

	/**
	 * @threadsafety Always.
	 */
	double operator()(const CheckableScheduleInfo& csi)
	{
		return csi.NextCheck;
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
			boost::multi_index::ordered_non_unique<CheckableNextCheckExtractor>
		>
	> CheckableSet;

	void OnConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	unsigned long GetIdleCheckables();
	unsigned long GetPendingCheckables();

private:
	std::mutex m_Mutex;
	std::condition_variable m_CV;
	bool m_Stopped{false};
	std::thread m_Thread;

	CheckableSet m_IdleCheckables;
	CheckableSet m_PendingCheckables;

	Timer::Ptr m_ResultTimer;

	void CheckThreadProc();
	void ResultTimerHandler();

	void ExecuteCheckHelper(const Checkable::Ptr& checkable);

	void AdjustCheckTimer();

	void ObjectHandler(const ConfigObject::Ptr& object);
	void NextCheckChangedHandler(const Checkable::Ptr& checkable);

	void RescheduleCheckTimer();

	static CheckableScheduleInfo GetCheckableScheduleInfo(const Checkable::Ptr& checkable);
};

}

#endif /* CHECKERCOMPONENT_H */
