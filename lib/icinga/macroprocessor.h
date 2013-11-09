/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef MACROPROCESSOR_H
#define MACROPROCESSOR_H

#include "icinga/i2-icinga.h"
#include "icinga/macroresolver.h"
#include "base/dictionary.h"
#include "base/array.h"
#include <boost/function.hpp>
#include <vector>

namespace icinga
{

/**
 * Resolves macros.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API MacroProcessor
{
public:
	typedef boost::function<String (const String&)> EscapeCallback;

	static Value ResolveMacros(const Value& str, const std::vector<MacroResolver::Ptr>& resolvers,
		const CheckResult::Ptr& cr, const EscapeCallback& escapeFn = EscapeCallback(), const Array::Ptr& escapeMacros = Array::Ptr());
	static bool ResolveMacro(const String& macro, const std::vector<MacroResolver::Ptr>& resolvers,
		const CheckResult::Ptr& cr, String *result);

private:
	MacroProcessor(void);

	static String InternalResolveMacros(const String& str,
		const std::vector<MacroResolver::Ptr>& resolvers, const CheckResult::Ptr& cr,
	    const EscapeCallback& escapeFn, const Array::Ptr& escapeMacros);
};

}

#endif /* MACROPROCESSOR_H */
