/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OBJECT_PACKER
#define OBJECT_PACKER

#include "base/i2-base.hpp"

namespace icinga
{

class String;
class Value;

String PackObject(const Value& value);

}

#endif /* OBJECT_PACKER */
