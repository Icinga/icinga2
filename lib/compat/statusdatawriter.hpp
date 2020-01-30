/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STATUSDATAWRITER_H
#define STATUSDATAWRITER_H

#include "base/timer.hpp"
#include "base/utility.hpp"
#include "compat/statusdatawriter-ti.hpp"
#include "icinga/command.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include <iostream>

namespace icinga
{

/**
 * @ingroup compat
 */
class StatusDataWriter final : public ObjectImpl<StatusDataWriter>
{
public:
	DECLARE_OBJECT(StatusDataWriter);
	DECLARE_OBJECTNAME(StatusDataWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	Timer::Ptr m_StatusTimer;
	bool m_ObjectsCacheOutdated;

	void DumpCommand(std::ostream& fp, const Command::Ptr& command);
	void DumpTimePeriod(std::ostream& fp, const TimePeriod::Ptr& tp);
	void DumpDowntimes(std::ostream& fp, const Checkable::Ptr& owner);
	void DumpComments(std::ostream& fp, const Checkable::Ptr& owner);
	void DumpHostStatus(std::ostream& fp, const Host::Ptr& host);
	void DumpHostObject(std::ostream& fp, const Host::Ptr& host);

	void DumpCheckableStatusAttrs(std::ostream& fp, const Checkable::Ptr& checkable);

	template<typename T>
	void DumpNameList(std::ostream& fp, const T& list)
	{
		bool first = true;
		for (const auto& obj : list) {
			if (!first)
				fp << ",";
			else
				first = false;

			fp << obj->GetName();
		}
	}

	template<typename T>
	void DumpStringList(std::ostream& fp, const T& list)
	{
		bool first = true;
		for (const auto& str : list) {
			if (!first)
				fp << ",";
			else
				first = false;

			fp << str;
		}
	}

	void DumpServiceStatus(std::ostream& fp, const Service::Ptr& service);
	void DumpServiceObject(std::ostream& fp, const Service::Ptr& service);

	void DumpCustomAttributes(std::ostream& fp, const CustomVarObject::Ptr& object);

	void UpdateObjectsCache();
	void StatusTimerHandler();
	void ObjectHandler();

	static String GetNotificationOptions(const Checkable::Ptr& checkable);
};

}

#endif /* STATUSDATAWRITER_H */
