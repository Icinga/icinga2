/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SCHEDULEDDOWNTIME_H
#define SCHEDULEDDOWNTIME_H

#include "icinga/i2-icinga.hpp"
#include "icinga/scheduleddowntime-ti.hpp"
#include "icinga/checkable.hpp"
#include <atomic>

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

	Checkable::Ptr GetCheckable() const;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);
	static bool AllConfigIsLoaded();

	void ValidateRanges(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateChildOptions(const Lazy<Value>& lvalue, const ValidationUtils& utils) override;
	String HashDowntimeOptions();

protected:
	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;

private:
	static void TimerProc();

	std::pair<double, double> FindRunningSegment(double minEnd = 0);
	std::pair<double, double> FindNextSegment();
	void CreateNextDowntime();
	void RemoveObsoleteDowntimes();

	static std::atomic<bool> m_AllConfigLoaded;

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule);
};

}

#endif /* SCHEDULEDDOWNTIME_H */
