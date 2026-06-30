// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef RANDOMCHECKTASK_H
#define RANDOMCHECKTASK_H

#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Test class for additional check types. Implements the "null" check type.
 *
 * @ingroup methods
 */
class RandomCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	RandomCheckTask();
};

}

#endif /* RANDOMCHECKTASK_H */
