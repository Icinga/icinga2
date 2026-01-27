// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SLEEPCHECKTASK_H
#define SLEEPCHECKTASK_H

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
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
    SleepCheckTask();
};

}

#endif /* SLEEPCHECKTASK_H */
