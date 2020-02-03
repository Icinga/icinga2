/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EVENTCOMMAND_H
#define EVENTCOMMAND_H

#include "icinga/checkable.hpp"
#include "icinga/eventcommand-ti.hpp"

namespace icinga
{

/**
 * An event handler command.
 *
 * @ingroup icinga
 */
class EventCommand final : public ObjectImpl<EventCommand>
{
public:
	DECLARE_OBJECT(EventCommand);
	DECLARE_OBJECTNAME(EventCommand);

	virtual void Execute(const Checkable::Ptr& checkable,
		const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false);
};

}

#endif /* EVENTCOMMAND_H */
