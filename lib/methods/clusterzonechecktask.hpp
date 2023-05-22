/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	ClusterZoneCheckTask();
};

}
