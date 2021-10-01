/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SYSLOGLOGGER_H
#define SYSLOGLOGGER_H

#ifndef _WIN32
#include "base/i2-base.hpp"
#include "base/sysloglogger-ti.hpp"

namespace icinga
{

/**
 * Helper class to handle syslog facility strings and numbers.
 *
 * @ingroup base
 */
class SyslogHelper final
{
public:
    static void StaticInitialize();
    static bool ValidateFacility(const String& facility);
    static int SeverityToNumber(LogSeverity severity);
    static int FacilityToNumber(const String& facility);

private:
    static std::map<String, int> m_FacilityMap;
};

/**
 * A logger that logs to syslog.
 *
 * @ingroup base
 */
class SyslogLogger final : public ObjectImpl<SyslogLogger>
{
public:
	DECLARE_OBJECT(SyslogLogger);
	DECLARE_OBJECTNAME(SyslogLogger);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void OnConfigLoaded() override;
	void ValidateFacility(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	int m_Facility;

	void ProcessLogEntry(const LogEntry& entry) override;
	void Flush() override;
};

}
#endif /* _WIN32 */

#endif /* SYSLOGLOGGER_H */
