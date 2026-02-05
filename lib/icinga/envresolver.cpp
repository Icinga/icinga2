// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/string.hpp"
#include "base/value.hpp"
#include "icinga/envresolver.hpp"
#include "icinga/checkresult.hpp"
#include <cstdlib>

using namespace icinga;

bool EnvResolver::ResolveMacro(const String& macro, const CheckResult::Ptr&, Value *result) const
{
	auto value (getenv(macro.CStr()));

	if (value) {
		*result = value;
	}

	return value;
}
