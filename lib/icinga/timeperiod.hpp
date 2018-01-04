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

#ifndef TIMEPERIOD_H
#define TIMEPERIOD_H

#include "icinga/i2-icinga.hpp"
#include "icinga/timeperiod.thpp"

namespace icinga
{

/**
 * A time period.
 *
 * @ingroup icinga
 */
class TimePeriod final : public ObjectImpl<TimePeriod>
{
public:
	DECLARE_OBJECT(TimePeriod);
	DECLARE_OBJECTNAME(TimePeriod);

	virtual void Start(bool runtimeCreated) override;

	void UpdateRegion(double begin, double end, bool clearExisting);

	virtual bool GetIsInside() const override;

	bool IsInside(double ts) const;
	double FindNextTransition(double begin);

	virtual void ValidateRanges(const Dictionary::Ptr& value, const ValidationUtils& utils) override;

private:
	void AddSegment(double s, double end);
	void AddSegment(const Dictionary::Ptr& segment);
	void RemoveSegment(double begin, double end);
	void RemoveSegment(const Dictionary::Ptr& segment);
	void PurgeSegments(double end);

	void Merge(const TimePeriod::Ptr& timeperiod, bool include = true);

	void Dump();

	static void UpdateTimerHandler();
};

}

#endif /* TIMEPERIOD_H */
