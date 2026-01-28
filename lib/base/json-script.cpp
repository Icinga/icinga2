// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	Namespace::Ptr jsonNS = new Namespace(true);

	/* Methods */
	jsonNS->Set("encode", new Function("Json#encode", JsonEncodeShim, { "value" }, true));
	jsonNS->Set("decode", new Function("Json#decode", JsonDecode, { "value" }, true));

	jsonNS->Freeze();

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");
	systemNS->Set("Json", jsonNS, true);
});
