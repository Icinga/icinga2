/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/i2-icinga.hpp"
#include "icinga/icingaapplication.thpp"
#include "icinga/macroresolver.hpp"

namespace icinga
{

/**
 * The Icinga application.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API IcingaApplication : public ObjectImpl<IcingaApplication>, public MacroResolver
{
public:
	DECLARE_OBJECT(IcingaApplication);
	DECLARE_OBJECTNAME(IcingaApplication);

	static void StaticInitialize(void);

	int Main(void);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static IcingaApplication::Ptr GetInstance(void);

	String GetPidPath(void) const;
	Dictionary::Ptr GetVars(void) const;
	String GetNodeName(void) const;

	virtual bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const;

	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled);
	void ClearEnableNotifications(void);

	bool GetEnableEventHandlers(void) const;
	void SetEnableEventHandlers(bool enabled);
	void ClearEnableEventHandlers(void);

	bool GetEnableFlapping(void) const;
	void SetEnableFlapping(bool enabled);
	void ClearEnableFlapping(void);

	bool GetEnableHostChecks(void) const;
	void SetEnableHostChecks(bool enabled);
	void ClearEnableHostChecks(void);
	bool GetEnableServiceChecks(void) const;
	void SetEnableServiceChecks(bool enabled);
	void ClearEnableServiceChecks(void);

	bool GetEnablePerfdata(void) const;
	void SetEnablePerfdata(bool enabled);
	void ClearEnablePerfdata(void);

private:
	void DumpProgramState(void);

	virtual void OnShutdown(void);
};

}

#endif /* ICINGAAPPLICATION_H */
