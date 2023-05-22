/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "icinga/i2-icinga.hpp"
#include "icinga/timeperiod-ti.hpp"

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

	void Start(bool runtimeCreated) override;

	void UpdateRegion(double begin, double end, bool clearExisting);

	bool GetIsInside() const override;

	bool IsInside(double ts) const;
	double FindNextTransition(double begin);

	void ValidateRanges(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

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
