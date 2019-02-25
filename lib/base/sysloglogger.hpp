/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SYSLOGLOGGER_H
#define SYSLOGLOGGER_H

#ifndef _WIN32
#include "base/i2-base.hpp"
#include "base/sysloglogger-ti.hpp"

namespace icinga
{

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

	static void StaticInitialize();
	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void OnConfigLoaded() override;
	void ValidateFacility(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	static std::map<String, int> m_FacilityMap;
	int m_Facility;

	void ProcessLogEntry(const LogEntry& entry) override;
	void Flush() override;
};

}
#endif /* _WIN32 */

#endif /* SYSLOGLOGGER_H */
