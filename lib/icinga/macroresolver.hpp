/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef MACRORESOLVER_H
#define MACRORESOLVER_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkresult.hpp"
#include "base/dictionary.hpp"
#include "base/string.hpp"

namespace icinga
{

/**
 * Resolves macros.
 *
 * @ingroup icinga
 */
class MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(MacroResolver);

	virtual bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const = 0;
};

}

#endif /* MACRORESOLVER_H */
