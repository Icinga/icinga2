// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/string.hpp"
#include "base/shared-object.hpp"
#include "base/configobject.hpp"

namespace icinga {

class ScriptPermissionChecker : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(ScriptPermissionChecker);

	ScriptPermissionChecker() = default;
	ScriptPermissionChecker(const ScriptPermissionChecker&) = delete;
	ScriptPermissionChecker(ScriptPermissionChecker&&) = delete;
	ScriptPermissionChecker& operator=(const ScriptPermissionChecker&) = delete;
	ScriptPermissionChecker& operator=(ScriptPermissionChecker&&) = delete;

	~ScriptPermissionChecker() override = default;

	virtual bool CanAccessGlobalVariable(const String& varName);
	virtual bool CanAccessConfigObject(const ConfigObject::Ptr& obj);
};

} // namespace icinga
