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

#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/debug.h"
#include "base/utility.h"
#include "base/timer.h"
#include "base/scriptvariable.h"
#include "base/initialize.h"

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);
INITIALIZE_ONCE(&IcingaApplication::StaticInitialize);

void IcingaApplication::StaticInitialize(void)
{
	ScriptVariable::Set("IcingaEnableNotifications", true);
	ScriptVariable::Set("IcingaEnableEventHandlers", true);
	ScriptVariable::Set("IcingaEnableFlapping", true);
	ScriptVariable::Set("IcingaEnableChecks", true);
	ScriptVariable::Set("IcingaEnablePerfdata", true);
}

/**
 * The entry point for the Icinga application.
 *
 * @returns An exit status.
 */
int IcingaApplication::Main(void)
{
	Log(LogDebug, "icinga", "In IcingaApplication::Main()");

	/* periodically dump the program state */
	l_RetentionTimer = make_shared<Timer>();
	l_RetentionTimer->SetInterval(300);
	l_RetentionTimer->OnTimerExpired.connect(boost::bind(&IcingaApplication::DumpProgramState, this));
	l_RetentionTimer->Start();

	RunEventLoop();

	Log(LogInformation, "icinga", "Icinga has shut down.");

	return EXIT_SUCCESS;
}

void IcingaApplication::OnShutdown(void)
{
	ASSERT(!OwnsLock());

	{
		ObjectLock olock(this);
		l_RetentionTimer->Stop();
	}

	DumpProgramState();
}

void IcingaApplication::DumpProgramState(void)
{
	DynamicObject::DumpObjects(GetStatePath());
}

IcingaApplication::Ptr IcingaApplication::GetInstance(void)
{
	return static_pointer_cast<IcingaApplication>(Application::GetInstance());
}

Dictionary::Ptr IcingaApplication::GetMacros(void) const
{
	return ScriptVariable::Get("IcingaMacros");
}

bool IcingaApplication::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	double now = Utility::GetTime();

	if (macro == "TIMET") {
		*result = Convert::ToString((long)now);
		return true;
	} else if (macro == "LONGDATETIME") {
		*result = Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", now);
		return true;
	} else if (macro == "SHORTDATETIME") {
		*result = Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", now);
		return true;
	} else if (macro == "DATE") {
		*result = Utility::FormatDateTime("%Y-%m-%d", now);
		return true;
	} else if (macro == "TIME") {
		*result = Utility::FormatDateTime("%H:%M:%S %z", now);
		return true;
	}

	Dictionary::Ptr macros = GetMacros();

	if (macros && macros->Contains(macro)) {
		*result = macros->Get(macro);
		return true;
	}

	return false;
}

bool IcingaApplication::GetEnableNotifications(void) const
{
	if (!GetOverrideEnableNotifications().IsEmpty())
		return GetOverrideEnableNotifications();
	else
		return ScriptVariable::Get("IcingaEnableNotifications");
}

void IcingaApplication::SetEnableNotifications(bool enabled)
{
	SetOverrideEnableNotifications(enabled);
}

void IcingaApplication::ClearEnableNotifications(void)
{
	SetOverrideEnableNotifications(Empty);
}

bool IcingaApplication::GetEnableEventHandlers(void) const
{
	if (!GetOverrideEnableEventHandlers().IsEmpty())
		return GetOverrideEnableEventHandlers();
	else
		return ScriptVariable::Get("IcingaEnableEventHandlers");
}

void IcingaApplication::SetEnableEventHandlers(bool enabled)
{
	SetOverrideEnableEventHandlers(enabled);
}

void IcingaApplication::ClearEnableEventHandlers(void)
{
	SetOverrideEnableEventHandlers(Empty);
}

bool IcingaApplication::GetEnableFlapping(void) const
{
	if (!GetOverrideEnableFlapping().IsEmpty())
		return GetOverrideEnableFlapping();
	else
		return ScriptVariable::Get("IcingaEnableFlapping");
}

void IcingaApplication::SetEnableFlapping(bool enabled)
{
	SetOverrideEnableFlapping(enabled);
}

void IcingaApplication::ClearEnableFlapping(void)
{
	SetOverrideEnableFlapping(Empty);
}

bool IcingaApplication::GetEnableChecks(void) const
{
	if (!GetOverrideEnableChecks().IsEmpty())
		return GetOverrideEnableChecks();
	else
		return ScriptVariable::Get("IcingaEnableChecks");
}

void IcingaApplication::SetEnableChecks(bool enabled)
{
	SetOverrideEnableChecks(enabled);
}

void IcingaApplication::ClearEnableChecks(void)
{
	SetOverrideEnableChecks(Empty);
}

bool IcingaApplication::GetEnablePerfdata(void) const
{
	if (!GetOverrideEnablePerfdata().IsEmpty())
		return GetOverrideEnablePerfdata();
	else
		return ScriptVariable::Get("IcingaEnablePerfdata");
}

void IcingaApplication::SetEnablePerfdata(bool enabled)
{
	SetOverrideEnablePerfdata(enabled);
}

void IcingaApplication::ClearEnablePerfdata(void)
{
	SetOverrideEnablePerfdata(Empty);
}
