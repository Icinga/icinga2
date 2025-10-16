/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base/scriptpermission.hpp"

using namespace icinga;

bool ScriptPermissionChecker::CanAccessGlobalVariable(const String&)
{
	return true;
}

bool ScriptPermissionChecker::CanAccessConfigObject(const ConfigObject::Ptr&)
{
	return true;
}
