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
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

static Timer::Ptr l_RetentionTimer;

REGISTER_TYPE(IcingaApplication);
INITIALIZE_ONCE(IcingaApplication, &IcingaApplication::StaticInitialize);

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

	m_StartTime = Utility::GetTime();

	/* periodically dump the program state */
	l_RetentionTimer = boost::make_shared<Timer>();
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

double IcingaApplication::GetStartTime(void) const
{
	ObjectLock olock(this);

	return m_StartTime;
}

bool IcingaApplication::ResolveMacro(const String& macro, const Dictionary::Ptr&, String *result) const
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
	if (!m_OverrideEnableNotifications.IsEmpty())
		return m_OverrideEnableNotifications;
	else
		return ScriptVariable::Get("IcingaEnableNotifications");
}

void IcingaApplication::SetEnableNotifications(bool enabled)
{
	m_OverrideEnableNotifications = enabled;
}

void IcingaApplication::ClearEnableNotifications(void)
{
	m_OverrideEnableNotifications = Empty;
}

bool IcingaApplication::GetEnableEventHandlers(void) const
{
	if (!m_OverrideEnableEventHandlers.IsEmpty())
		return m_OverrideEnableEventHandlers;
	else
		return ScriptVariable::Get("IcingaEnableEventHandlers");
}

void IcingaApplication::SetEnableEventHandlers(bool enabled)
{
	m_OverrideEnableEventHandlers = enabled;
}

void IcingaApplication::ClearEnableEventHandlers(void)
{
	m_OverrideEnableEventHandlers = Empty;
}

bool IcingaApplication::GetEnableFlapping(void) const
{
	if (!m_OverrideEnableFlapping.IsEmpty())
		return m_OverrideEnableFlapping;
	else
		return ScriptVariable::Get("IcingaEnableFlapping");
}

void IcingaApplication::SetEnableFlapping(bool enabled)
{
	m_OverrideEnableFlapping = enabled;
}

void IcingaApplication::ClearEnableFlapping(void)
{
	m_OverrideEnableFlapping = Empty;
}

bool IcingaApplication::GetEnableChecks(void) const
{
	if (!m_OverrideEnableChecks.IsEmpty())
		return m_OverrideEnableChecks;
	else
		return ScriptVariable::Get("IcingaEnableChecks");
}

void IcingaApplication::SetEnableChecks(bool enabled)
{
	m_OverrideEnableChecks = enabled;
}

void IcingaApplication::ClearEnableChecks(void)
{
	m_OverrideEnableChecks = Empty;
}

bool IcingaApplication::GetEnablePerfdata(void) const
{
	if (!m_OverrideEnablePerfdata.IsEmpty())
		return m_OverrideEnablePerfdata;
	else
		return ScriptVariable::Get("IcingaEnablePerfdata");
}

void IcingaApplication::SetEnablePerfdata(bool enabled)
{
	m_OverrideEnablePerfdata = enabled;
}

void IcingaApplication::ClearEnablePerfdata(void)
{
	m_OverrideEnablePerfdata = Empty;
}

void IcingaApplication::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_State) {
		bag->Set("override_enable_notifications", m_OverrideEnableNotifications);
		bag->Set("override_enable_event_handlers", m_OverrideEnableEventHandlers);
		bag->Set("override_enable_flapping", m_OverrideEnableFlapping);
		bag->Set("override_enable_checks", m_OverrideEnableChecks);
		bag->Set("override_enable_perfdata", m_OverrideEnablePerfdata);
	}
}

void IcingaApplication::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_State) {
		m_OverrideEnableNotifications = bag->Get("override_enable_notifications");
		m_OverrideEnableEventHandlers = bag->Get("override_enable_event_handlers");
		m_OverrideEnableFlapping = bag->Get("override_enable_flapping");
		m_OverrideEnableChecks = bag->Get("override_enable_checks");
		m_OverrideEnablePerfdata = bag->Get("override_enable_perfdata");
	}
}
