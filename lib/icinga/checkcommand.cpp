/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkcommand.hpp"
#include "icinga/checkcommand-ti.cpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_TYPE(CheckCommand);

void CheckCommand::Execute(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	GetExecute()->Invoke({
		checkable,
		cr,
		resolvedMacros,
		useResolvedMacros
	});
}
