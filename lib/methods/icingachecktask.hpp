/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ICINGACHECKTASK_H
#define ICINGACHECKTASK_H

#include "icinga/service.hpp"
#include "methods/i2-methods.hpp"

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
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IcingaCheckTask();
};

}

#endif /* ICINGACHECKTASK_H */
