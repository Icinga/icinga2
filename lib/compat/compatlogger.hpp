// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COMPATLOGGER_H
#define COMPATLOGGER_H

#include "compat/compatlogger-ti.hpp"
#include "icinga/service.hpp"
#include "base/timer.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga compat log writer.
 *
 * @ingroup compat
 */
class CompatLogger final : public ObjectImpl<CompatLogger>
{
public:
	DECLARE_OBJECT(CompatLogger);
	DECLARE_OBJECTNAME(CompatLogger);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidateRotationMethod(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	void WriteLine(const String& line);
	void Flush();

	void CheckResultHandler(const Checkable::Ptr& service, const CheckResult::Ptr& cr);
	void NotificationSentHandler(const Checkable::Ptr& service,
		const User::Ptr& user, NotificationType notification_type, CheckResult::Ptr const& cr,
		const String& author, const String& comment_text, const String& command_name);
	void FlappingChangedHandler(const Checkable::Ptr& checkable);
	void EnableFlappingChangedHandler(const Checkable::Ptr& checkable);
	void TriggerDowntimeHandler(const Downtime::Ptr& downtime);
	void RemoveDowntimeHandler(const Downtime::Ptr& downtime);
	void ExternalCommandHandler(const String& command, const std::vector<String>& arguments);
	void EventCommandHandler(const Checkable::Ptr& service);

	static String GetHostStateString(const Host::Ptr& host);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler();
	void ScheduleNextRotation();

	std::ofstream m_OutputFile;
	void ReopenFile(bool rotate);
};

}

#endif /* COMPATLOGGER_H */
