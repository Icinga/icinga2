// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/customvarobject.hpp"
#include "icinga/customvarobject-ti.cpp"
#include "icinga/macroprocessor.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_TYPE(CustomVarObject);

void CustomVarObject::ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils&)
{
	MacroProcessor::ValidateCustomVars(this, lvalue());
}
