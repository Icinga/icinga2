/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#include "base/sysloglogger.hpp"
#include "base/sysloglogger-ti.cpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

REGISTER_TYPE(SyslogLogger);

REGISTER_STATSFUNCTION(SyslogLogger, &SyslogLogger::StatsFunc);

INITIALIZE_ONCE(&SyslogLogger::StaticInitialize);

std::map<String, int> SyslogLogger::m_FacilityMap;

void SyslogLogger::StaticInitialize()
{
	ScriptGlobal::Set("System.FacilityAuth", "LOG_AUTH", true);
	ScriptGlobal::Set("System.FacilityAuthPriv", "LOG_AUTHPRIV", true);
	ScriptGlobal::Set("System.FacilityCron", "LOG_CRON", true);
	ScriptGlobal::Set("System.FacilityDaemon", "LOG_DAEMON", true);
	ScriptGlobal::Set("System.FacilityFtp", "LOG_FTP", true);
	ScriptGlobal::Set("System.FacilityKern", "LOG_KERN", true);
	ScriptGlobal::Set("System.FacilityLocal0", "LOG_LOCAL0", true);
	ScriptGlobal::Set("System.FacilityLocal1", "LOG_LOCAL1", true);
	ScriptGlobal::Set("System.FacilityLocal2", "LOG_LOCAL2", true);
	ScriptGlobal::Set("System.FacilityLocal3", "LOG_LOCAL3", true);
	ScriptGlobal::Set("System.FacilityLocal4", "LOG_LOCAL4", true);
	ScriptGlobal::Set("System.FacilityLocal5", "LOG_LOCAL5", true);
	ScriptGlobal::Set("System.FacilityLocal6", "LOG_LOCAL6", true);
	ScriptGlobal::Set("System.FacilityLocal7", "LOG_LOCAL7", true);
	ScriptGlobal::Set("System.FacilityLpr", "LOG_LPR", true);
	ScriptGlobal::Set("System.FacilityMail", "LOG_MAIL", true);
	ScriptGlobal::Set("System.FacilityNews", "LOG_NEWS", true);
	ScriptGlobal::Set("System.FacilitySyslog", "LOG_SYSLOG", true);
	ScriptGlobal::Set("System.FacilityUser", "LOG_USER", true);
	ScriptGlobal::Set("System.FacilityUucp", "LOG_UUCP", true);

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
	DictionaryData nodes;

	for (const SyslogLogger::Ptr& sysloglogger : ConfigType::GetObjectsByType<SyslogLogger>()) {
		nodes.emplace_back(sysloglogger->GetName(), 1); //add more stats
	}

	status->Set("sysloglogger", new Dictionary(std::move(nodes)));
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

void SyslogLogger::ValidateFacility(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<SyslogLogger>::ValidateFacility(lvalue, utils);

	if (m_FacilityMap.find(lvalue()) == m_FacilityMap.end()) {
		try {
			Convert::ToLong(lvalue());
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
