/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/dependency-ti.hpp"

namespace icinga
{

class ApplyRule;
struct ScriptFrame;
class Host;
class Service;

/**
 * A service dependency..
 *
 * @ingroup icinga
 */
class Dependency final : public ObjectImpl<Dependency>
{
public:
	DECLARE_OBJECT(Dependency);
	DECLARE_OBJECTNAME(Dependency);

	intrusive_ptr<Checkable> GetParent() const;
	intrusive_ptr<Checkable> GetChild() const;

	TimePeriod::Ptr GetPeriod() const;

	bool IsAvailable(DependencyType dt) const;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);
	static void AssertNoCycles();

	/* Note: Only use them for unit test mocks. Prefer OnConfigLoaded(). */
	void SetParent(intrusive_ptr<Checkable> parent);
	void SetChild(intrusive_ptr<Checkable> child);

protected:
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Stop(bool runtimeRemoved) override;

private:
	Checkable::Ptr m_Parent;
	Checkable::Ptr m_Child;

	static bool m_AssertNoCyclesForIndividualDeps;

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule, bool skipFilter = false);
};

}

#endif /* DEPENDENCY_H */
