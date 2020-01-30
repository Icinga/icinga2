/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Test class for additional check types. Implements the "null" check type.
 *
 * @ingroup methods
 */
class NullCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	NullCheckTask();
};

}
