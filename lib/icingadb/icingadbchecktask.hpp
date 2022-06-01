/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#ifndef ICINGADBCHECKTASK_H
#define ICINGADBCHECKTASK_H

#include "icingadb/icingadb.hpp"
#include "icinga/checkable.hpp"

namespace icinga
{

/**
 * Icinga DB check.
 *
 * @ingroup icingadb
 */
class IcingadbCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IcingadbCheckTask();
};

}

#endif /* ICINGADBCHECKTASK_H */
