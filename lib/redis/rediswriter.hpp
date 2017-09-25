/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef REDISWRITER_H
#define REDISWRITER_H

#include "redis/rediswriter.thpp"
#include "remote/messageorigin.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include <hiredis/hiredis.h>

namespace icinga
{

struct RedisSubscriptionInfo
{
	std::set<String> EventTypes;
};

/**
 * @ingroup redis
 */
class RedisWriter : public ObjectImpl<RedisWriter>
{
public:
	DECLARE_OBJECT(RedisWriter);
	DECLARE_OBJECTNAME(RedisWriter);

	RedisWriter(void);

	static void ConfigStaticInitialize(void);

	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

private:
	void ReconnectTimerHandler(void);
	void TryToReconnect(void);
	void HandleEvents(void);
	void HandleEvent(const Dictionary::Ptr& event);

	void UpdateSubscriptionsTimerHandler(void);
	void UpdateSubscriptions(void);
	void PublishStatsTimerHandler(void);
	void PublishStats(void);

	/* config & status dump */
	void UpdateAllConfigObjects(void);
	void SendConfigUpdate(const ConfigObject::Ptr& object, const String& typeName, bool runtimeUpdate = false);
	void SendConfigDelete(const ConfigObject::Ptr& object, const String& typeName);
	void SendStatusUpdate(const ConfigObject::Ptr& object, const String& typeName);

	/* utilities */
	static String FormatCheckSumBinary(const String& str);

	static String CalculateCheckSumString(const String& str, bool binary = false);
	static String CalculateCheckSumGroups(const Array::Ptr& groups, bool binary = false);
	static String CalculateCheckSumProperties(const ConfigObject::Ptr& object, bool binary = false);
	static String CalculateCheckSumVars(const ConfigObject::Ptr& object, bool binary = false);

	static String HashValue(const Value& value, bool binary = false);
	static Dictionary::Ptr SerializeObjectAttrs(const Object::Ptr& object, int fieldType);

	static void StateChangedHandler(const ConfigObject::Ptr& object);
	static void VarsChangedHandler(const ConfigObject::Ptr& object);
	static void VersionChangedHandler(const ConfigObject::Ptr& object);

	void AssertOnWorkQueue(void);

	void ExceptionHandler(boost::exception_ptr exp);

	boost::shared_ptr<redisReply> ExecuteQuery(const std::vector<String>& query);

	Timer::Ptr m_StatsTimer;
	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_SubscriptionTimer;
	WorkQueue m_WorkQueue;
	redisContext *m_Context;
	std::map<String, RedisSubscriptionInfo> m_Subscriptions;
	bool m_ConfigDumpInProgress;
};

struct redis_error : virtual std::exception, virtual boost::exception { };

struct errinfo_redis_query_;
typedef boost::error_info<struct errinfo_redis_query_, std::string> errinfo_redis_query;

}

#endif /* REDISWRITER_H */
