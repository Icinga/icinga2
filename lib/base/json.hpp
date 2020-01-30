/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"

namespace icinga
{

class String;
class Value;

String JsonEncode(const Value& value, bool pretty_print = false);
Value JsonDecode(const String& data);

}
