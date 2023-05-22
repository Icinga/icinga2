/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/dbconnection.hpp"
#include "icinga/checkable.hpp"

namespace icinga
{

/**
 * IDO check type.
 *
 * @ingroup db_ido
 */
class IdoCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IdoCheckTask();
};

}
