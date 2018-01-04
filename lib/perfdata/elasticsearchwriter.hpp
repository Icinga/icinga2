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

#ifndef ELASTICSEARCHWRITER_H
#define ELASTICSEARCHWRITER_H

#include "perfdata/elasticsearchwriter.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/workqueue.hpp"
#include "base/timer.hpp"

namespace icinga
{

class ElasticsearchWriter final : public ObjectImpl<ElasticsearchWriter>
{
public:
	DECLARE_OBJECT(ElasticsearchWriter);
	DECLARE_OBJECTNAME(ElasticsearchWriter);

	ElasticsearchWriter();

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static String FormatTimestamp(double ts);

protected:
	void OnConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	String m_EventPrefix;
	WorkQueue m_WorkQueue;
	Timer::Ptr m_FlushTimer;
	std::vector<String> m_DataBuffer;
	boost::mutex m_DataBufferMutex;

	void AddCheckResult(const Dictionary::Ptr& fields, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);

	void StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);
	void StateChangeHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type);
	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void InternalCheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void NotificationSentToAllUsersHandler(const Notification::Ptr& notification,
		const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
		const CheckResult::Ptr& cr, const String& author, const String& text);
	void NotificationSentToAllUsersHandlerInternal(const Notification::Ptr& notification,
		const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
		const CheckResult::Ptr& cr, const String& author, const String& text);

	void Enqueue(const String& type, const Dictionary::Ptr& fields, double ts);

	Stream::Ptr Connect();
	void AssertOnWorkQueue();
	void ExceptionHandler(boost::exception_ptr exp);
	void FlushTimeout();
	void Flush();
	void SendRequest(const String& body);
};

}

#endif /* ELASTICSEARCHWRITER_H */
