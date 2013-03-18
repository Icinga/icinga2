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

#ifndef TIMEPERIOD_H
#define TIMEPERIOD_H

#include "icinga/i2-icinga.h"
#include "base/dynamicobject.h"
#include "base/array.h"

namespace icinga
{

/**
 * A time period.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API TimePeriod : public DynamicObject
{
public:
	typedef shared_ptr<TimePeriod> Ptr;
	typedef weak_ptr<TimePeriod> WeakPtr;

	explicit TimePeriod(const Dictionary::Ptr& serializedUpdate);

	static TimePeriod::Ptr GetByName(const String& name);

	virtual void Start(void);

	void UpdateRegion(double begin, double end);

	bool IsInside(double ts) const;
	double FindNextTransition(double begin);

	static void EmptyTimePeriodUpdate(const ScriptTask::Ptr& task, const std::vector<Value>& arguments);
	static void EvenMinutesTimePeriodUpdate(const ScriptTask::Ptr& task, const std::vector<Value>& arguments);

private:
	Attribute<double> m_ValidBegin;
	Attribute<double> m_ValidEnd;
	Attribute<Array::Ptr> m_Segments;

	void AddSegment(double s, double end);
	void AddSegment(const Dictionary::Ptr& segment);
	void RemoveSegment(double begin, double end);
	void PurgeSegments(double end);

	static void UpdateTimerHandler(void);
};

}

#endif /* TIMEPERIOD_H */
