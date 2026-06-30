// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DUMMYCHECKTASK_H
#define DUMMYCHECKTASK_H

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Test class for additional check types. Implements the "dummy" check type.
 *
 * @ingroup methods
 */
class DummyCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	DummyCheckTask();
};

}

#endif /* DUMMYCHECKTASK_H */
