/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/function.hpp"
#include "base/serializer.hpp"

using namespace icinga;

REGISTER_FUNCTION(Internal, serialize, &Serialize, "value:attribute_types");
