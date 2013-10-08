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

#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

#include "icinga/i2-icinga.h"
#include "icinga/macroresolver.h"
#include "base/application.h"

namespace icinga
{

/**
 * The Icinga application.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API IcingaApplication : public Application, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(IcingaApplication);
	DECLARE_TYPENAME(IcingaApplication);

	static void StaticInitialize(void);

	int Main(void);

	static IcingaApplication::Ptr GetInstance(void);

	String GetPidPath(void) const;
	Dictionary::Ptr GetMacros(void) const;

	double GetStartTime(void) const;

	virtual bool ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const;

	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled);
	void ClearEnableNotifications(void);

	bool GetEnableEventHandlers(void) const;
	void SetEnableEventHandlers(bool enabled);
	void ClearEnableEventHandlers(void);

	bool GetEnableFlapping(void) const;
	void SetEnableFlapping(bool enabled);
	void ClearEnableFlapping(void);

	bool GetEnableChecks(void) const;
	void SetEnableChecks(bool enabled);
	void ClearEnableChecks(void);

	bool GetEnablePerfdata(void) const;
	void SetEnablePerfdata(bool enabled);
	void ClearEnablePerfdata(void);

protected:
	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	double m_StartTime;

	Value m_OverrideEnableNotifications;
	Value m_OverrideEnableEventHandlers;
	Value m_OverrideEnableFlapping;
	Value m_OverrideEnableChecks;
	Value m_OverrideEnablePerfdata;

	void DumpProgramState(void);

	virtual void OnShutdown(void);
};

}

#endif /* ICINGAAPPLICATION_H */
