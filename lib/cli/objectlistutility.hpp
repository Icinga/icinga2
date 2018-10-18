/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef OBJECTLISTUTILITY_H
#define OBJECTLISTUTILITY_H

#include "base/i2-base.hpp"
#include "cli/i2-cli.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/value.hpp"
#include "base/string.hpp"

namespace icinga
{

/**
 * @ingroup cli
 */
class ObjectListUtility
{
public:
	static bool PrintObject(std::ostream& fp, bool& first, const String& message, std::map<String, int>& type_count, const String& name_filter, const String& type_filter);

private:
	static void PrintProperties(std::ostream& fp, const Dictionary::Ptr& props, const Dictionary::Ptr& debug_hints, int indent);
	static void PrintHints(std::ostream& fp, const Dictionary::Ptr& debug_hints, int indent);
	static void PrintHint(std::ostream& fp, const Array::Ptr& msg, int indent);
	static void PrintValue(std::ostream& fp, const Value& val);
	static void PrintArray(std::ostream& fp, const Array::Ptr& arr);
};

}

#endif /* OBJECTLISTUTILITY_H */
