/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#ifndef JOURNALDLOGGER_H
#define JOURNALDLOGGER_H

#include "base/i2-base.hpp"
#if !defined(_WIN32) && defined(HAVE_SYSTEMD)
#include "base/journaldlogger-ti.hpp"
#include <sys/uio.h>

namespace icinga
{

/**
 * A logger that logs to systemd journald.
 *
 * @ingroup base
 */
class JournaldLogger final : public ObjectImpl<JournaldLogger>
{
public:
	DECLARE_OBJECT(JournaldLogger);
	DECLARE_OBJECTNAME(JournaldLogger);

	static void StaticInitialize();
	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void OnConfigLoaded() override;
	void ValidateFacility(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	void SystemdJournalSend(const std::vector<String>& varJournalFields) const;
	static struct iovec IovecFromString(const String& s);

	std::vector<String> m_ConfiguredJournalFields;

	void ProcessLogEntry(const LogEntry& entry) override;
	void Flush() override;
};

}
#endif /* !_WIN32 && HAVE_SYSTEMD */

#endif /* JOURNALDLOGGER_H */
