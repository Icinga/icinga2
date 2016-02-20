/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

	virtual int Main(void) override;

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static IcingaApplication::Ptr GetInstance(void);

	String GetPidPath(void) const;

	virtual bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, Value *result) const override;

	String GetNodeName(void) const;

	virtual void ValidateVars(const Dictionary::Ptr& value, const ValidationUtils& utils) override;

private:
	void DumpProgramState(void);
	void DumpModifiedAttributes(void);

	virtual void OnShutdown(void) override;
};

}

#endif /* ICINGAAPPLICATION_H */
