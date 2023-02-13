/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#ifndef ICINGA4WINAPICHECKTASK_H
#define ICINGA4WINAPICHECKTASK_H

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Executes checks via local Icinga for Windows API.
 *
 * @ingroup methods
 */
class IfwApiCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IfwApiCheckTask();
};

}

#endif /* ICINGA4WINAPICHECKTASK_H */
