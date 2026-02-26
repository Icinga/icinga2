// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ELASTICSEARCHWRITER_H
#define ELASTICSEARCHWRITER_H

#include "perfdata/elasticsearchwriter-ti.hpp"
#include "icinga/checkable.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"
#include "perfdata/perfdatawriterconnection.hpp"

namespace icinga
{

class ElasticsearchWriter final : public ObjectImpl<ElasticsearchWriter>
{
public:
	DECLARE_OBJECT(ElasticsearchWriter);
	DECLARE_OBJECTNAME(ElasticsearchWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static String FormatTimestamp(double ts);

	void ValidateHostTagsTemplate(const Lazy<Dictionary::Ptr> &lvalue, const ValidationUtils &utils) override;
	void ValidateServiceTagsTemplate(const Lazy<Dictionary::Ptr> &lvalue, const ValidationUtils &utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	WorkQueue m_WorkQueue{10000000, 1};
	boost::signals2::connection m_HandleCheckResults, m_HandleStateChanges, m_HandleNotifications;
	Timer::Ptr m_FlushTimer;
	std::atomic_bool m_FlushTimerInQueue{false};
	std::vector<String> m_DataBuffer;
	PerfdataWriterConnection::Ptr m_Connection;

	void AddCheckResult(const Dictionary::Ptr& fields, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void AddTemplateTags(const Dictionary::Ptr& fields, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);

	void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void NotificationSentToAllUsersHandler(const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
		NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text);

	void Enqueue(const Checkable::Ptr& checkable, const String& type,
		const Dictionary::Ptr& fields, double ts);

	void AssertOnWorkQueue();
	void ExceptionHandler(boost::exception_ptr exp);
	void FlushTimeout();
	void Flush();
	void SendRequest(const String& body);
};

}

#endif /* ELASTICSEARCHWRITER_H */
