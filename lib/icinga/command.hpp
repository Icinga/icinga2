/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef COMMAND_H
#define COMMAND_H

#include "icinga/command-ti.hpp"
#include "icinga/i2-icinga.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * A command.
 *
 * @ingroup icinga
 */
class Command : public ObjectImpl<Command>
{
public:
	DECLARE_OBJECT(Command);

	//virtual Dictionary::Ptr Execute(const Object::Ptr& context) = 0;

	void Validate(int types, const ValidationUtils& utils) override;
};

}

#endif /* COMMAND_H */
