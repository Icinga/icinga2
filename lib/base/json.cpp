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

#include "base/json.hpp"
#include "base/debug.hpp"
#include "base/namespace.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <boost/exception_ptr.hpp>
#include <yajl/yajl_version.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>
#include <json.hpp>
#include <stack>

using namespace icinga;

using json = nlohmann::json;

String icinga::JsonEncode(const Value& value, bool pretty_print)
{
	String result;

	switch (value.GetType()) {
		case ValueNumber:
		case ValueBoolean:
		case ValueString:
		case ValueObject:
		case ValueEmpty:
		default:
			VERIFY(!"Invalid variant type.");
	}

	return result;
}

Value icinga::JsonDecode(const String& data)
{
	
}
