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

#ifndef REDISWRITER_H
#define REDISWRITER_H

#include "redis/rediswriter-ti.hpp"
#include "icinga/customvarobject.hpp"
#include "remote/messageorigin.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "redis/redisconnection.hpp"
#include "icinga/checkable.hpp"
#include "icinga/service.hpp"
#include "icinga/downtime.hpp"
#include <memory>
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

	RedisWriter();

	static void ConfigStaticInitialize();

	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

private:
	void ReconnectTimerHandler();
	void TryToReconnect();
	void HandleEvents();
	void HandleEvent(const Dictionary::Ptr& event);
	void SendEvent(const Dictionary::Ptr& event);

	void UpdateSubscriptionsTimerHandler();
	void UpdateSubscriptions();
	bool GetSubscriptionTypes(String key, RedisSubscriptionInfo& rsi);
	void PublishStatsTimerHandler();
	void PublishStats();

	/* config & status dump */
	void UpdateAllConfigObjects();
	std::vector<std::vector<intrusive_ptr<ConfigObject>>> ChunkObjects(std::vector<intrusive_ptr<ConfigObject>> objects, size_t chunkSize);
	void DeleteKeys(const std::vector<String>& keys);
	std::vector<String> GetTypeObjectKeys(const String& type);
	void InsertObjectDependencies(const ConfigObject::Ptr& object, const String typeName, std::map<String, std::vector<String> >& statements);
	void UpdateState(const Checkable::Ptr& checkable);
	void SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate);
	void CreateConfigUpdate(const ConfigObject::Ptr& object, const String type, std::map<String, std::vector<String> >& statements,
			bool runtimeUpdate);
	void SendConfigDelete(const ConfigObject::Ptr& object);
	void SendStatusUpdate(const ConfigObject::Ptr& object);
	std::vector<String> UpdateObjectAttrs(const ConfigObject::Ptr& object, int fieldType, const String& typeNameOverride);
	Dictionary::Ptr SerializeState(const Checkable::Ptr& checkable);

	/* Stats */
	Dictionary::Ptr GetStats();

	/* utilities */
	static String FormatCheckSumBinary(const String& str);
	static String FormatCommandLine(const Value& commandLine);

	static String GetObjectIdentifier(const ConfigObject::Ptr& object);
	static String GetEnvironment();
	static String CalculateCheckSumString(const String& str);
	static String CalculateCheckSumArray(const Array::Ptr& arr);
	static String CalculateCheckSumProperties(const ConfigObject::Ptr& object, const std::set<String>& propertiesBlacklist);
	static String CalculateCheckSumMetadata(const ConfigObject::Ptr& object);
	static String CalculateCheckSumVars(const CustomVarObject::Ptr& object);
	static Dictionary::Ptr SerializeVars(const CustomVarObject::Ptr& object);

	static String HashValue(const Value& value);
	static String HashValue(const Value& value, const std::set<String>& propertiesBlacklist, bool propertiesWhitelist = false);

	static String GetLowerCaseTypeNameDB(const ConfigObject::Ptr& obj);
	static bool PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes, Dictionary::Ptr& checkSums);

	static void StateChangeHandler(const ConfigObject::Ptr& object);
	static void VersionChangedHandler(const ConfigObject::Ptr& object);
	static void DowntimeChangedHandler(const Downtime::Ptr& downtime);

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);

	//Used to get a reply from the asyncronous connection
	std::shared_ptr<redisReply> RedisGet(const std::vector<String>& query);
	static void RedisQueryCallback(redisAsyncContext *c, void *r, void *p);
	static redisReply* dupReplyObject(redisReply* reply);


	Timer::Ptr m_StatsTimer;
	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_SubscriptionTimer;
	WorkQueue m_WorkQueue;
	std::map<String, RedisSubscriptionInfo> m_Subscriptions;

	String m_PrefixConfigObject;
	String m_PrefixConfigCheckSum;
	String m_PrefixStateObject;

	bool m_ConfigDumpInProgress;
	bool m_ConfigDumpDone;

	RedisConnection::Ptr m_Rcon;
};
}

#endif /* REDISWRITER_H */
