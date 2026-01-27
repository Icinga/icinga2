// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CLUSTERZONECHECKTASK_H
#define CLUSTERZONECHECKTASK_H

#include "icinga/service.hpp"

namespace icinga
{

/**
 * Cluster zone check type.
 *
 * @ingroup methods
 */
class ClusterZoneCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	ClusterZoneCheckTask();
};

}

#endif /* CLUSTERZONECHECKTASK_H */
