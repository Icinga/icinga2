// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/application.hpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

String Application::GetAppEnvironment()
{
	Value defaultValue = Empty;
	return ScriptGlobal::Get("Environment", &defaultValue);
}

void Application::SetAppEnvironment(const String& name)
{
	ScriptGlobal::Set("Environment", name);
}
