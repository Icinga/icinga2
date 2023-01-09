/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#include "base/sysloglogger.hpp"
#include "base/sysloglogger-ti.cpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include <syslog.h>

using namespace icinga;

REGISTER_TYPE(SyslogLogger);

REGISTER_STATSFUNCTION(SyslogLogger, &SyslogLogger::StatsFunc);

INITIALIZE_ONCE(&SyslogHelper::StaticInitialize);

std::map<String, int> SyslogHelper::m_FacilityMap;

void SyslogHelper::StaticInitialize()
{
	ScriptGlobal::Set("System.FacilityAuth", "LOG_AUTH");
	ScriptGlobal::Set("System.FacilityAuthPriv", "LOG_AUTHPRIV");
	ScriptGlobal::Set("System.FacilityCron", "LOG_CRON");
	ScriptGlobal::Set("System.FacilityDaemon", "LOG_DAEMON");
	ScriptGlobal::Set("System.FacilityFtp", "LOG_FTP");
	ScriptGlobal::Set("System.FacilityKern", "LOG_KERN");
	ScriptGlobal::Set("System.FacilityLocal0", "LOG_LOCAL0");
	ScriptGlobal::Set("System.FacilityLocal1", "LOG_LOCAL1");
	ScriptGlobal::Set("System.FacilityLocal2", "LOG_LOCAL2");
	ScriptGlobal::Set("System.FacilityLocal3", "LOG_LOCAL3");
	ScriptGlobal::Set("System.FacilityLocal4", "LOG_LOCAL4");
	ScriptGlobal::Set("System.FacilityLocal5", "LOG_LOCAL5");
	ScriptGlobal::Set("System.FacilityLocal6", "LOG_LOCAL6");
	ScriptGlobal::Set("System.FacilityLocal7", "LOG_LOCAL7");
	ScriptGlobal::Set("System.FacilityLpr", "LOG_LPR");
	ScriptGlobal::Set("System.FacilityMail", "LOG_MAIL");
	ScriptGlobal::Set("System.FacilityNews", "LOG_NEWS");
	ScriptGlobal::Set("System.FacilitySyslog", "LOG_SYSLOG");
	ScriptGlobal::Set("System.FacilityUser", "LOG_USER");
	ScriptGlobal::Set("System.FacilityUucp", "LOG_UUCP");

	m_FacilityMap["LOG_AUTH"] = LOG_AUTH;
	m_FacilityMap["LOG_AUTHPRIV"] = LOG_AUTHPRIV;
	m_FacilityMap["LOG_CRON"] = LOG_CRON;
	m_FacilityMap["LOG_DAEMON"] = LOG_DAEMON;
#ifdef LOG_FTP
	m_FacilityMap["LOG_FTP"] = LOG_FTP;
#endif /* LOG_FTP */
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

bool SyslogHelper::ValidateFacility(const String& facility)
{
	if (m_FacilityMap.find(facility) == m_FacilityMap.end()) {
		try {
			Convert::ToLong(facility);
		} catch (const std::exception&) {
			return false;
		}
	}
	return true;
}

int SyslogHelper::SeverityToNumber(LogSeverity severity)
{
	switch (severity) {
	case LogDebug:
		return LOG_DEBUG;
	case LogNotice:
		return LOG_NOTICE;
	case LogWarning:
		return LOG_WARNING;
	case LogCritical:
		return LOG_CRIT;
	case LogInformation:
	default:
		return LOG_INFO;
	}
}

int SyslogHelper::FacilityToNumber(const String& facility)
{
	auto it = m_FacilityMap.find(facility);
	if (it != m_FacilityMap.end())
		return it->second;
	else
		return Convert::ToLong(facility);
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
	m_Facility = SyslogHelper::FacilityToNumber(GetFacility());
}

void SyslogLogger::ValidateFacility(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<SyslogLogger>::ValidateFacility(lvalue, utils);
	if (!SyslogHelper::ValidateFacility(lvalue()))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "facility" }, "Invalid facility specified."));
}

/**
 * Processes a log entry and outputs it to syslog.
 *
 * @param entry The log entry.
 */
void SyslogLogger::ProcessLogEntry(const LogEntry& entry)
{
	syslog(SyslogHelper::SeverityToNumber(entry.Severity) | m_Facility,
		"%s", entry.Message.CStr());
}

void SyslogLogger::Flush()
{
	/* Nothing to do here. */
}
#endif /* _WIN32 */
