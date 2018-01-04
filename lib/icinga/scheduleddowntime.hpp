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

#ifndef SCHEDULEDDOWNTIME_H
#define SCHEDULEDDOWNTIME_H

#include "icinga/i2-icinga.hpp"
#include "icinga/scheduleddowntime.thpp"
#include "icinga/checkable.hpp"
#include <utility>

namespace icinga
{

class ApplyRule;
struct ScriptFrame;
class Host;
class Service;

/**
 * An Icinga scheduled downtime specification.
 *
 * @ingroup icinga
 */
class ScheduledDowntime final : public ObjectImpl<ScheduledDowntime>
{
public:
	DECLARE_OBJECT(ScheduledDowntime);
	DECLARE_OBJECTNAME(ScheduledDowntime);

	Checkable::Ptr GetCheckable(void) const;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

	virtual void ValidateRanges(const Dictionary::Ptr& value, const ValidationUtils& utils) override;

protected:
	virtual void OnAllConfigLoaded(void) override;
	virtual void Start(bool runtimeCreated) override;

private:
	static void TimerProc(void);

	std::pair<double, double> FindNextSegment(void);
	void CreateNextDowntime(void);

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule);
};

}

#endif /* SCHEDULEDDOWNTIME_H */
