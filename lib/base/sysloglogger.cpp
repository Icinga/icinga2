/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef _WIN32
#include "base/sysloglogger.hpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include "base/sysloglogger.tcpp"

using namespace icinga;

REGISTER_TYPE(SyslogLogger);

REGISTER_STATSFUNCTION(SyslogLogger, &SyslogLogger::StatsFunc);

INITIALIZE_ONCE(&SyslogLogger::StaticInitialize);

std::map<String, int> SyslogLogger::m_FacilityMap;

void SyslogLogger::StaticInitialize()
{
	ScriptGlobal::Set("FacilityAuth", "LOG_AUTH");
	ScriptGlobal::Set("FacilityAuthPriv", "LOG_AUTHPRIV");
	ScriptGlobal::Set("FacilityCron", "LOG_CRON");
	ScriptGlobal::Set("FacilityDaemon", "LOG_DAEMON");
	ScriptGlobal::Set("FacilityFtp", "LOG_FTP");
	ScriptGlobal::Set("FacilityKern", "LOG_KERN");
	ScriptGlobal::Set("FacilityLocal0", "LOG_LOCAL0");
	ScriptGlobal::Set("FacilityLocal1", "LOG_LOCAL1");
	ScriptGlobal::Set("FacilityLocal2", "LOG_LOCAL2");
	ScriptGlobal::Set("FacilityLocal3", "LOG_LOCAL3");
	ScriptGlobal::Set("FacilityLocal4", "LOG_LOCAL4");
	ScriptGlobal::Set("FacilityLocal5", "LOG_LOCAL5");
	ScriptGlobal::Set("FacilityLocal6", "LOG_LOCAL6");
	ScriptGlobal::Set("FacilityLocal7", "LOG_LOCAL7");
	ScriptGlobal::Set("FacilityLpr", "LOG_LPR");
	ScriptGlobal::Set("FacilityMail", "LOG_MAIL");
	ScriptGlobal::Set("FacilityNews", "LOG_NEWS");
	ScriptGlobal::Set("FacilitySyslog", "LOG_SYSLOG");
	ScriptGlobal::Set("FacilityUser", "LOG_USER");
	ScriptGlobal::Set("FacilityUucp", "LOG_UUCP");

	m_FacilityMap["LOG_AUTH"] = LOG_AUTH;
	m_FacilityMap["LOG_AUTHPRIV"] = LOG_AUTHPRIV;
	m_FacilityMap["LOG_CRON"] = LOG_CRON;
	m_FacilityMap["LOG_DAEMON"] = LOG_DAEMON;
	m_FacilityMap["LOG_FTP"] = LOG_FTP;
	m_FacilityMap["LOG_KERN"] = LOG_KERN;
	m_FacilityMap["LOG_LOCAL0"] = LOG_LOCAL0;
	m_FacilityMap["LOG_LOCAL1"] = LOG_LOCAL1;
	m_FacilityMap["LOG_LOCAL2"] = LOG_LOCAL2;
	m_FacilityMap["LOG_LOCAL3"] = LOG_LOCAL3;
	m_FacilityMap["LOG_LOCAL4"] = LOG_LOCAL4;
	m_FacilityMap["LOG_LOCAL5"] = LOG_LOCAL5;
	m_FacilityMap["LOG_LOCAL6"] = LOG_LOCAL6;
	m_FacilityMap["LOG_LOCAL7"] = LOG_LOCAL7;
	m_FacilityMap["LOG_LPR"] = LOG_LPR;
	m_FacilityMap["LOG_MAIL"] = LOG_MAIL;
	m_FacilityMap["LOG_NEWS"] = LOG_NEWS;
	m_FacilityMap["LOG_SYSLOG"] = LOG_SYSLOG;
	m_FacilityMap["LOG_USER"] = LOG_USER;
	m_FacilityMap["LOG_UUCP"] = LOG_UUCP;
}

void SyslogLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const SyslogLogger::Ptr& sysloglogger : ConfigType::GetObjectsByType<SyslogLogger>()) {
		nodes->Set(sysloglogger->GetName(), 1); //add more stats
	}

	status->Set("sysloglogger", nodes);
}

void SyslogLogger::OnConfigLoaded()
{
	ObjectImpl<SyslogLogger>::OnConfigLoaded();

	String facilityString = GetFacility();

	auto it = m_FacilityMap.find(facilityString);

	if (it != m_FacilityMap.end())
		m_Facility = it->second;
	else
		m_Facility = Convert::ToLong(facilityString);
}

void SyslogLogger::ValidateFacility(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<SyslogLogger>::ValidateFacility(value, utils);

	if (m_FacilityMap.find(value) == m_FacilityMap.end()) {
		try {
			Convert::ToLong(value);
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "facility" }, "Invalid facility specified."));
		}
	}
}

/**
 * Processes a log entry and outputs it to syslog.
 *
 * @param entry The log entry.
 */
void SyslogLogger::ProcessLogEntry(const LogEntry& entry)
{
	int severity;
	switch (entry.Severity) {
		case LogDebug:
			severity = LOG_DEBUG;
			break;
		case LogNotice:
			severity = LOG_NOTICE;
			break;
		case LogWarning:
			severity = LOG_WARNING;
			break;
		case LogCritical:
			severity = LOG_CRIT;
			break;
		case LogInformation:
		default:
			severity = LOG_INFO;
			break;
	}

	syslog(severity | m_Facility, "%s", entry.Message.CStr());
}

void SyslogLogger::Flush()
{
	/* Nothing to do here. */
}
#endif /* _WIN32 */
