/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSON_H
#define JSON_H

#include "base/i2-base.hpp"

namespace icinga
{

class String;
class Value;

static constexpr size_t JsonDecodeDefaultDepthLimit = 24;

String JsonEncode(const Value& value, bool pretty_print = false);
Value JsonDecode(const String& data, size_t depthLimit = JsonDecodeDefaultDepthLimit);

}

#endif /* JSON_H */
