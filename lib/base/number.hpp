/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NUMBER_H
#define NUMBER_H

#include "base/i2-base.hpp"
#include "base/object.hpp"

namespace icinga {

class Value;

/**
 * Number class.
 */
class Number
{
public:
	static Object::Ptr GetPrototype();

private:
	Number();
};

}

#endif /* NUMBER_H */
