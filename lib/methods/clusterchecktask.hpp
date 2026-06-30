// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CLUSTERCHECKTASK_H
#define CLUSTERCHECKTASK_H

#include "icinga/service.hpp"

namespace icinga
{

/**
 * Cluster check type.
 *
 * @ingroup methods
 */
class ClusterCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	ClusterCheckTask();
	static String FormatArray(const Array::Ptr& arr);
};

}

#endif /* CLUSTERCHECKTASK_H */
