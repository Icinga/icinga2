/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/eventcommand.hpp"
#include "icinga/eventcommand-ti.cpp"

using namespace icinga;

REGISTER_TYPE(EventCommand);

void EventCommand::Execute(const Checkable::Ptr& checkable,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	GetExecute()->Invoke({
		checkable,
		resolvedMacros,
		useResolvedMacros
	});
}
