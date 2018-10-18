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

#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/initialize.hpp"
#include "base/json.hpp"

using namespace icinga;

static String JsonEncodeShim(const Value& value)
{
	return JsonEncode(value);
}

INITIALIZE_ONCE([]() {
	auto jsonNSBehavior = new ConstNamespaceBehavior();
	Namespace::Ptr jsonNS = new Namespace(jsonNSBehavior);

	/* Methods */
	jsonNS->Set("encode", new Function("Json#encode", JsonEncodeShim, { "value" }, true));
	jsonNS->Set("decode", new Function("Json#decode", JsonDecode, { "value" }, true));

	jsonNSBehavior->Freeze();

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");
	systemNS->SetAttribute("Json", std::make_shared<ConstEmbeddedNamespaceValue>(jsonNS));
});
