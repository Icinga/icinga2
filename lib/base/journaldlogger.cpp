/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "base/i2-base.hpp"
#if !defined(_WIN32) && defined(HAVE_SYSTEMD)
#include "base/journaldlogger.hpp"
#include "base/journaldlogger-ti.cpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include "base/sysloglogger.hpp"
#include <systemd/sd-journal.h>

using namespace icinga;

REGISTER_TYPE(JournaldLogger);

REGISTER_STATSFUNCTION(JournaldLogger, &JournaldLogger::StatsFunc);

void JournaldLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	DictionaryData nodes;

	for (const JournaldLogger::Ptr& journaldlogger : ConfigType::GetObjectsByType<JournaldLogger>()) {
		nodes.emplace_back(journaldlogger->GetName(), 1); //add more stats
	}

	status->Set("journaldlogger", new Dictionary(std::move(nodes)));
}

void JournaldLogger::OnConfigLoaded()
{
	ObjectImpl<JournaldLogger>::OnConfigLoaded();
	m_ConfiguredJournalFields.clear();
	m_ConfiguredJournalFields.push_back(
		String("SYSLOG_FACILITY=") + Value(SyslogHelper::FacilityToNumber(GetFacility())));
	const String identifier = GetIdentifier();
	if (!identifier.IsEmpty()) {
		m_ConfiguredJournalFields.push_back(String("SYSLOG_IDENTIFIER=" + identifier));
	}
}

void JournaldLogger::ValidateFacility(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<JournaldLogger>::ValidateFacility(lvalue, utils);
	if (!SyslogHelper::ValidateFacility(lvalue()))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "facility" }, "Invalid facility specified."));
}

/**
 * Processes a log entry and outputs it to journald.
 *
 * @param entry The log entry.
 */
void JournaldLogger::ProcessLogEntry(const LogEntry& entry)
{
	const std::vector<String> sdFields {
		String("MESSAGE=") + entry.Message.GetData(),
		String("PRIORITY=") + Value(SyslogHelper::SeverityToNumber(entry.Severity)),
		String("ICINGA2_FACILITY=") + entry.Facility,
	};
	SystemdJournalSend(sdFields);
}

void JournaldLogger::Flush()
{
	/* Nothing to do here. */
}

void JournaldLogger::SystemdJournalSend(const std::vector<String>& varJournalFields) const
{
	struct iovec iovec[m_ConfiguredJournalFields.size() + varJournalFields.size()];
	int iovecCount = 0;

	for (const String& journalField: m_ConfiguredJournalFields) {
		iovec[iovecCount] = IovecFromString(journalField);
		iovecCount++;
	}
	for (const String& journalField: varJournalFields) {
		iovec[iovecCount] = IovecFromString(journalField);
		iovecCount++;
	}
	sd_journal_sendv(iovec, iovecCount);
}

struct iovec JournaldLogger::IovecFromString(const String& s) {
	return { const_cast<char *>(s.CStr()), s.GetLength() };
}
#endif /* !_WIN32 && HAVE_SYSTEMD */
