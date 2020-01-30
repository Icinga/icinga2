/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "icinga/i2-icinga.hpp"
#include "icinga/icingaapplication-ti.hpp"
#include "icinga/macroresolver.hpp"

namespace icinga
{

/**
 * The Icinga application.
 *
 * @ingroup icinga
 */
class IcingaApplication final : public ObjectImpl<IcingaApplication>, public MacroResolver
{
public:
	DECLARE_OBJECT(IcingaApplication);
	DECLARE_OBJECTNAME(IcingaApplication);

	static void StaticInitialize();

	int Main() override;

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static IcingaApplication::Ptr GetInstance();

	bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const override;

	String GetNodeName() const;

	int GetMaxConcurrentChecks() const;

	String GetEnvironment() const override;
	void SetEnvironment(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;

	void ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

private:
	void DumpProgramState();
	void DumpModifiedAttributes();

	void OnShutdown() override;
};

}
