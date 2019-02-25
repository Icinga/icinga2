/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
