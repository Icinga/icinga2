// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ICINGACHECKTASK_H
#define ICINGACHECKTASK_H

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"

namespace icinga
{

/**
 * Icinga check type.
 *
 * @ingroup methods
 */
class IcingaCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IcingaCheckTask();
};

}

#endif /* ICINGACHECKTASK_H */
