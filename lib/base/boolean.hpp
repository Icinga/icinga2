/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef BOOLEAN_H
#define BOOLEAN_H

#include "base/i2-base.hpp"
#include "base/object.hpp"

namespace icinga {

class Value;

/**
 * Boolean class.
 */
class Boolean
{
public:
	static Object::Ptr GetPrototype();

private:
	Boolean();
};

}

#endif /* BOOLEAN_H */
