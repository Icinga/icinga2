/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Test class for additional event handler types. Implements the "null" event handler type.
 *
 * @ingroup methods
 */
class NullEventTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	NullEventTask();
};

}
