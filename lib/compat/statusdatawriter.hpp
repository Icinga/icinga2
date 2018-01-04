/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef STATUSDATAWRITER_H
#define STATUSDATAWRITER_H

#include "compat/statusdatawriter.thpp"
#include "icinga/customvarobject.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/command.hpp"
#include "icinga/compatutility.hpp"
#include "base/timer.hpp"
#include "base/utility.hpp"
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
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

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
};

}

#endif /* STATUSDATAWRITER_H */
