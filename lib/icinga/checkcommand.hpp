/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKCOMMAND_H
#define CHECKCOMMAND_H

#include "icinga/checkcommand-ti.hpp"
#include "icinga/checkable.hpp"

namespace icinga
{

/**
 * A command.
 *
 * @ingroup icinga
 */
class CheckCommand final : public ObjectImpl<CheckCommand>
{
public:
	DECLARE_OBJECT(CheckCommand);
	DECLARE_OBJECTNAME(CheckCommand);

	static thread_local CheckCommand::Ptr ExecuteOverride;

	void Execute(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer,
		const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false);
};

}

#endif /* CHECKCOMMAND_H */
