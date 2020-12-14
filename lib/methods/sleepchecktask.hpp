/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Test class for additional check types. Implements the "sleep" check type.
 *
 * @ingroup methods
 */
class SleepCheckTask
{
public:
    static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
            const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
    SleepCheckTask();
};

}