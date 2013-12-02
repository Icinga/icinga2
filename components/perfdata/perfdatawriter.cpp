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

#include "perfdata/perfdatawriter.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/context.h"
#include "base/application.h"

using namespace icinga;

REGISTER_TYPE(PerfdataWriter);

void PerfdataWriter::Start(void)
{
	DynamicObject::Start();

	Service::OnNewCheckResult.connect(boost::bind(&PerfdataWriter::CheckResultHandler, this, _1, _2));

	m_RotationTimer = make_shared<Timer>();
	m_RotationTimer->OnTimerExpired.connect(boost::bind(&PerfdataWriter::RotationTimerHandler, this));
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile();
}

void PerfdataWriter::CheckResultHandler(const Service::Ptr& service, const CheckResult::Ptr& cr)
{
	CONTEXT("Writing performance data for service '" + service->GetShortName() + "' on host '" + service->GetHost()->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !service->GetEnablePerfdata())
		return;

	Host::Ptr host = service->GetHost();

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(host);
	resolvers.push_back(IcingaApplication::GetInstance());

	String line = MacroProcessor::ResolveMacros(GetFormatTemplate(), resolvers, cr);

	ObjectLock olock(this);
	if (!m_OutputFile.good())
		return;

	m_OutputFile << line << "\n";
}

void PerfdataWriter::RotateFile(void)
{
	ObjectLock olock(this);

	String tempFile = GetTempPath();

	if (m_OutputFile.good()) {
		m_OutputFile.close();

		String finalFile = GetPerfdataPath() + "." + Convert::ToString((long)Utility::GetTime());
		(void) rename(tempFile.CStr(), finalFile.CStr());
	}

	m_OutputFile.open(tempFile.CStr());

	if (!m_OutputFile.good())
		Log(LogWarning, "icinga", "Could not open perfdata file '" + tempFile + "' for writing. Perfdata will be lost.");
}

void PerfdataWriter::RotationTimerHandler(void)
{
	RotateFile();
}

