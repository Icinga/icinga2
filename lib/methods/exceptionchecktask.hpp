/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXCEPTIONCHECKTASK_H
#define EXCEPTIONCHECKTASK_H

#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Test class for additional check types. Implements the "exception" check type.
 *
 * @ingroup methods
 */
class ExceptionCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	ExceptionCheckTask();
};

}

#endif /* EXCEPTIONCHECKTASK_H */
