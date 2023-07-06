/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#pragma once

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Executes checks via Icinga for Windows API.
 *
 * @ingroup methods
 */
class IfwApiCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IfwApiCheckTask();
};

}
