// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
