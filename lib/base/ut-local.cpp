/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/shared-object.hpp"
#include <unordered_map>

namespace icinga
{
namespace UT
{

thread_local std::unordered_map<void*, SharedObject::Ptr> l_KernelspaceThreadLocals;

}
}
