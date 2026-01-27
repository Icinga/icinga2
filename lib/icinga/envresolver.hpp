// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ENVRESOLVER_H
#define ENVRESOLVER_H

#include "base/object.hpp"
#include "base/string.hpp"
#include "base/value.hpp"
#include "icinga/macroresolver.hpp"
#include "icinga/checkresult.hpp"

namespace icinga
{

/**
 * Resolves env var names.
 *
 * @ingroup icinga
 */
class EnvResolver final : public Object, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(EnvResolver);

	bool ResolveMacro(const String& macro, const CheckResult::Ptr&, Value *result) const override;
};

}

#endif /* ENVRESOLVER_H */
