/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadb.hpp"
#include "icingadb/redisconnection.hpp"
#include "base/configtype.hpp"
#include "base/configobject.hpp"
#include "base/defer.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/shared.hpp"
#include "base/tlsutility.hpp"
#include "base/initialize.hpp"
#include "base/convert.hpp"
#include "base/array.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include "base/object-packer.hpp"
#include "icinga/command.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/dependency.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/usergroup.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/pluginutility.hpp"
#include "remote/zone.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <type_traits>

using namespace icinga;

using Prio = RedisConnection::QueryPriority;

std::unordered_set<Type*> IcingaDB::m_IndexedTypes;

INITIALIZE_ONCE(&IcingaDB::ConfigStaticInitialize);

std::vector<Type::Ptr> IcingaDB::GetTypes()
{
	// The initial config sync will queue the types in the following order.
	return {
		// Sync them first to get their states ASAP.
		Host::TypeInstance,
		Service::TypeInstance,

		// Then sync them for similar reasons.
		Downtime::TypeInstance,
		Comment::TypeInstance,

		HostGroup::TypeInstance,
		ServiceGroup::TypeInstance,
		CheckCommand::TypeInstance,
		Endpoint::TypeInstance,
		EventCommand::TypeInstance,
		Notification::TypeInstance,
		NotificationCommand::TypeInstance,
		TimePeriod::TypeInstance,
		User::TypeInstance,
		UserGroup::TypeInstance,
		Zone::TypeInstance
	};
}

void IcingaDB::ConfigStaticInitialize()
{
	for (auto& type : GetTypes()) {
		m_IndexedTypes.emplace(type.get());
	}

	/* triggered in ProcessCheckResult(), requires UpdateNextCheck() to be called before */
	Checkable::OnStateChange.connect([](const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type, const MessageOrigin::Ptr&) {
		IcingaDB::StateChangeHandler(checkable, cr, type);
	});

	Checkable::OnAcknowledgementSet.connect([](const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool, bool persistent, double changeTime, double expiry, const MessageOrigin::Ptr&) {
		AcknowledgementSetHandler(checkable, author, comment, type, persistent, changeTime, expiry);
	});
	Checkable::OnAcknowledgementCleared.connect([](const Checkable::Ptr& checkable, const String& removedBy, double changeTime, const MessageOrigin::Ptr&) {
		AcknowledgementClearedHandler(checkable, removedBy, changeTime);
	});

	Checkable::OnReachabilityChanged.connect([](const Checkable::Ptr& parent, const CheckResult::Ptr&, std::set<Checkable::Ptr>, const MessageOrigin::Ptr&) {
		// Icinga DB Web needs to know about the reachability of all children, not just the direct ones.
		// These might get updated with their next check result anyway, but we can't rely on that, since
		// they might not be actively checked or have a very high check interval.
		IcingaDB::ReachabilityChangeHandler(parent->GetAllChildren());
	});

	/* triggered on create, update and delete objects */
	ConfigObject::OnActiveChanged.connect([](const ConfigObject::Ptr& object, const Value&) {
		IcingaDB::VersionChangedHandler(object);
	});
	ConfigObject::OnVersionChanged.connect([](const ConfigObject::Ptr& object, const Value&) {
		IcingaDB::VersionChangedHandler(object);
	});

	DependencyGroup::OnChildRegistered.connect(&IcingaDB::DependencyGroupChildRegisteredHandler);
	DependencyGroup::OnChildRemoved.connect(&IcingaDB::DependencyGroupChildRemovedHandler);

	/* downtime start */
	Downtime::OnDowntimeTriggered.connect(&IcingaDB::DowntimeStartedHandler);
	/* fixed/flexible downtime end or remove */
	Downtime::OnDowntimeRemoved.connect(&IcingaDB::DowntimeRemovedHandler);

	Checkable::OnNotificationSentToAllUsers.connect([](
		const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
		const NotificationType& type, const CheckResult::Ptr& cr, const String& author, const String& text,
		const MessageOrigin::Ptr&
	) {
		IcingaDB::NotificationSentToAllUsersHandler(notification, checkable, users, type, cr, author, text);
	});

	Comment::OnCommentAdded.connect(&IcingaDB::CommentAddedHandler);
	Comment::OnCommentRemoved.connect(&IcingaDB::CommentRemovedHandler);

	Checkable::OnFlappingChange.connect(&IcingaDB::FlappingChangeHandler);

	Checkable::OnNewCheckResult.connect([](const Checkable::Ptr& checkable, const CheckResult::Ptr&, const MessageOrigin::Ptr&) {
		IcingaDB::NewCheckResultHandler(checkable);
	});

	Checkable::OnNextCheckUpdated.connect([](const Checkable::Ptr& checkable) {
		IcingaDB::NextCheckUpdatedHandler(checkable);
	});

	Service::OnHostProblemChanged.connect([](const Service::Ptr& service, const CheckResult::Ptr&, const MessageOrigin::Ptr&) {
		IcingaDB::HostProblemChangedHandler(service);
	});

	Notification::OnUsersRawChangedWithOldValue.connect([](const Notification::Ptr& notification, const Value& oldValues, const Value& newValues) {
		IcingaDB::NotificationUsersChangedHandler(notification, oldValues, newValues);
	});
	Notification::OnUserGroupsRawChangedWithOldValue.connect([](const Notification::Ptr& notification, const Value& oldValues, const Value& newValues) {
		IcingaDB::NotificationUserGroupsChangedHandler(notification, oldValues, newValues);
	});
	TimePeriod::OnRangesChangedWithOldValue.connect([](const TimePeriod::Ptr& timeperiod, const Value& oldValues, const Value& newValues) {
		IcingaDB::TimePeriodRangesChangedHandler(timeperiod, oldValues, newValues);
	});
	TimePeriod::OnIncludesChangedWithOldValue.connect([](const TimePeriod::Ptr& timeperiod, const Value& oldValues, const Value& newValues) {
		IcingaDB::TimePeriodIncludesChangedHandler(timeperiod, oldValues, newValues);
	});
	TimePeriod::OnExcludesChangedWithOldValue.connect([](const TimePeriod::Ptr& timeperiod, const Value& oldValues, const Value& newValues) {
		IcingaDB::TimePeriodExcludesChangedHandler(timeperiod, oldValues, newValues);
	});
	User::OnGroupsChangedWithOldValue.connect([](const User::Ptr& user, const Value& oldValues, const Value& newValues) {
		IcingaDB::UserGroupsChangedHandler(user, oldValues, newValues);
	});
	Host::OnGroupsChangedWithOldValue.connect([](const Host::Ptr& host, const Value& oldValues, const Value& newValues) {
		IcingaDB::HostGroupsChangedHandler(host, oldValues, newValues);
	});
	Service::OnGroupsChangedWithOldValue.connect([](const Service::Ptr& service, const Value& oldValues, const Value& newValues) {
		IcingaDB::ServiceGroupsChangedHandler(service, oldValues, newValues);
	});
	Command::OnEnvChangedWithOldValue.connect([](const ConfigObject::Ptr& command, const Value& oldValues, const Value& newValues) {
		IcingaDB::CommandEnvChangedHandler(command, oldValues, newValues);
	});
	Command::OnArgumentsChangedWithOldValue.connect([](const ConfigObject::Ptr& command, const Value& oldValues, const Value& newValues) {
		IcingaDB::CommandArgumentsChangedHandler(command, oldValues, newValues);
	});
	CustomVarObject::OnVarsChangedWithOldValue.connect([](const ConfigObject::Ptr& object, const Value& oldValues, const Value& newValues) {
		IcingaDB::CustomVarsChangedHandler(object, oldValues, newValues);
	});
}

void IcingaDB::UpdateAllConfigObjects()
{
	m_Rcon->Sync();
	m_Rcon->FireAndForgetQuery({"XADD", "icinga:schema", "MAXLEN", "1", "*", "version", "6"}, Prio::Heartbeat);

	Log(LogInformation, "IcingaDB") << "Starting initial config/status dump";
	double startTime = Utility::GetTime();

	SetOngoingDumpStart(startTime);

	Defer resetOngoingDumpStart ([this]() {
		SetOngoingDumpStart(0);
	});

	// Use a Workqueue to pack objects in parallel
	WorkQueue upq(25000, Configuration::Concurrency, LogNotice);
	upq.SetName("IcingaDB:ConfigDump");

	std::vector<Type::Ptr> types = GetTypes();

	m_Rcon->SuppressQueryKind(Prio::CheckResult);
	m_Rcon->SuppressQueryKind(Prio::RuntimeStateSync);

	Defer unSuppress ([this]() {
		m_Rcon->UnsuppressQueryKind(Prio::RuntimeStateSync);
		m_Rcon->UnsuppressQueryKind(Prio::CheckResult);
	});

	// Add a new type=* state=wip entry to the stream and remove all previous entries (MAXLEN 1).
	m_Rcon->FireAndForgetQuery({"XADD", "icinga:dump", "MAXLEN", "1", "*", "key", "*", "state", "wip"}, Prio::Config);

	const std::vector<String> globalKeys = {
		m_PrefixConfigObject + "customvar",
		m_PrefixConfigObject + "action:url",
		m_PrefixConfigObject + "notes:url",
		m_PrefixConfigObject + "icon:image",

		// These keys aren't tied to a specific Checkable type but apply to both "Host" and "Service" types,
		// and as such we've to make sure to clear them before we actually start dumping the actual objects.
		// This allows us to wait on both types to be dumped before we send a config dump done signal for those keys.
		m_PrefixConfigObject + "dependency:node",
		m_PrefixConfigObject + "dependency:edge",
		m_PrefixConfigObject + "dependency:edge:state",
		m_PrefixConfigObject + "redundancygroup",
		m_PrefixConfigObject + "redundancygroup:state",
	};
	DeleteKeys(m_Rcon, globalKeys, Prio::Config);
	DeleteKeys(m_Rcon, {"icinga:nextupdate:host", "icinga:nextupdate:service"}, Prio::Config);
	m_Rcon->Sync();

	Defer resetDumpedGlobals ([this]() {
		m_DumpedGlobals.CustomVar.Reset();
		m_DumpedGlobals.ActionUrl.Reset();
		m_DumpedGlobals.NotesUrl.Reset();
		m_DumpedGlobals.IconImage.Reset();
		m_DumpedGlobals.DependencyGroup.Reset();
	});

	upq.ParallelFor(types, false, [this](const Type::Ptr& type) {
		String lcType = type->GetName().ToLower();
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());
		if (!ctype)
			return;

		auto& rcon (m_Rcons.at(ctype));

		std::vector<String> keys = GetTypeOverwriteKeys(lcType);
		DeleteKeys(rcon, keys, Prio::Config);

		WorkQueue upqObjectType(25000, Configuration::Concurrency, LogNotice);
		upqObjectType.SetName("IcingaDB:ConfigDump:" + lcType);

		std::map<String, String> redisCheckSums;
		String configCheckSum = m_PrefixConfigCheckSum + lcType;

		upqObjectType.Enqueue([&rcon, &configCheckSum, &redisCheckSums]() {
			String cursor = "0";

			do {
				Array::Ptr res = rcon->GetResultOfQuery({
					"HSCAN", configCheckSum, cursor, "COUNT", "1000"
				}, Prio::Config);

				AddKvsToMap(res->Get(1), redisCheckSums);

				cursor = res->Get(0);
			} while (cursor != "0");
		});

		auto objectChunks (ChunkObjects(ctype->GetObjects(), 500));
		String configObject = m_PrefixConfigObject + lcType;

		// Skimmed away attributes and checksums HMSETs' keys and values by Redis key.
		std::map<String, std::vector<std::vector<String>>> ourContentRaw {{configCheckSum, {}}, {configObject, {}}};
		std::mutex ourContentMutex;

		upqObjectType.ParallelFor(objectChunks, [&](decltype(objectChunks)::const_reference chunk) {
			std::map<String, std::vector<String>> hMSets;
			std::vector<String> hostZAdds = {"ZADD", "icinga:nextupdate:host"}, serviceZAdds = {"ZADD", "icinga:nextupdate:service"};

			auto skimObjects ([&]() {
				std::lock_guard<std::mutex> l (ourContentMutex);

				for (auto& kv : ourContentRaw) {
					auto pos (hMSets.find(kv.first));

					if (pos != hMSets.end()) {
						kv.second.emplace_back(std::move(pos->second));
						hMSets.erase(pos);
					}
				}
			});

			bool dumpState = (lcType == "host" || lcType == "service");

			size_t bulkCounter = 0;
			for (const ConfigObject::Ptr& object : chunk) {
				if (lcType != GetLowerCaseTypeNameDB(object))
					continue;

				// If we encounter not yet activated objects, i.e. they are currently being loaded and are about to
				// be activated, but are still partially initialised, we want to exclude them from the config dump
				// before we end up in a nullptr deference and crash the Icinga 2 process. Should these excluded
				// objects later reach the activation process, they will be captured via the `OnActiveChanged` event
				// and processed in IcingaDB::VersionChangedHandler() as runtime updates.
				if (!object->IsActive()) {
					continue;
				}

				std::vector<Dictionary::Ptr> runtimeUpdates;
				CreateConfigUpdate(object, lcType, hMSets, runtimeUpdates, false);

				// Write out inital state for checkables
				if (dumpState) {
					String objectKey = GetObjectIdentifier(object);
					Dictionary::Ptr state = SerializeState(dynamic_pointer_cast<Checkable>(object));

					auto& states = hMSets[m_PrefixConfigObject + lcType + ":state"];
					states.emplace_back(objectKey);
					states.emplace_back(JsonEncode(state));

					auto& statesChksms = hMSets[m_PrefixConfigCheckSum + lcType + ":state"];
					statesChksms.emplace_back(objectKey);
					statesChksms.emplace_back(JsonEncode(new Dictionary({{"checksum", HashValue(state)}})));
				}

				bulkCounter++;
				if (!(bulkCounter % 100)) {
					skimObjects();

					ExecuteRedisTransaction(rcon, hMSets, {});

					hMSets = decltype(hMSets)();
				}

				auto checkable (dynamic_pointer_cast<Checkable>(object));

				if (checkable && checkable->GetEnableActiveChecks()) {
					auto zAdds (dynamic_pointer_cast<Service>(checkable) ? &serviceZAdds : &hostZAdds);

					zAdds->emplace_back(Convert::ToString(checkable->GetNextUpdate()));
					zAdds->emplace_back(GetObjectIdentifier(checkable));

					if (zAdds->size() >= 102u) {
						std::vector<String> header (zAdds->begin(), zAdds->begin() + 2u);

						rcon->FireAndForgetQuery(std::move(*zAdds), Prio::CheckResult);

						*zAdds = std::move(header);
					}
				}
			}

			skimObjects();

			ExecuteRedisTransaction(rcon, hMSets, {});

			for (auto zAdds : {&hostZAdds, &serviceZAdds}) {
				if (zAdds->size() > 2u) {
					rcon->FireAndForgetQuery(std::move(*zAdds), Prio::CheckResult);
				}
			}

			Log(LogNotice, "IcingaDB")
					<< "Dumped " << bulkCounter << " objects of type " << lcType;
		});

		upqObjectType.Join();

		if (upqObjectType.HasExceptions()) {
			for (boost::exception_ptr exc : upqObjectType.GetExceptions()) {
				if (exc) {
					boost::rethrow_exception(exc);
				}
			}
		}

		std::map<String, std::map<String, String>> ourContent;

		for (auto& source : ourContentRaw) {
			auto& dest (ourContent[source.first]);

			upqObjectType.Enqueue([&]() {
				for (auto& hMSet : source.second) {
					for (decltype(hMSet.size()) i = 0, stop = hMSet.size() - 1u; i < stop; i += 2u) {
						dest.emplace(std::move(hMSet[i]), std::move(hMSet[i + 1u]));
					}

					hMSet.clear();
				}

				source.second.clear();
			});
		}

		upqObjectType.Join();
		ourContentRaw.clear();

		auto& ourCheckSums (ourContent[configCheckSum]);
		auto& ourObjects (ourContent[configObject]);
		std::vector<String> setChecksum, setObject, delChecksum, delObject;

		auto redisCurrent (redisCheckSums.begin());
		auto redisEnd (redisCheckSums.end());
		auto ourCurrent (ourCheckSums.begin());
		auto ourEnd (ourCheckSums.end());

		auto flushSets ([&]() {
			auto affectedConfig (setObject.size() / 2u);

			setChecksum.insert(setChecksum.begin(), {"HMSET", configCheckSum});
			setObject.insert(setObject.begin(), {"HMSET", configObject});

			std::vector<std::vector<String>> transaction;

			transaction.emplace_back(std::vector<String>{"MULTI"});
			transaction.emplace_back(std::move(setChecksum));
			transaction.emplace_back(std::move(setObject));
			transaction.emplace_back(std::vector<String>{"EXEC"});

			setChecksum.clear();
			setObject.clear();

			rcon->FireAndForgetQueries(std::move(transaction), Prio::Config, {affectedConfig});
		});

		auto flushDels ([&]() {
			auto affectedConfig (delObject.size());

			delChecksum.insert(delChecksum.begin(), {"HDEL", configCheckSum});
			delObject.insert(delObject.begin(), {"HDEL", configObject});

			std::vector<std::vector<String>> transaction;

			transaction.emplace_back(std::vector<String>{"MULTI"});
			transaction.emplace_back(std::move(delChecksum));
			transaction.emplace_back(std::move(delObject));
			transaction.emplace_back(std::vector<String>{"EXEC"});

			delChecksum.clear();
			delObject.clear();

			rcon->FireAndForgetQueries(std::move(transaction), Prio::Config, {affectedConfig});
		});

		auto setOne ([&]() {
			setChecksum.emplace_back(ourCurrent->first);
			setChecksum.emplace_back(ourCurrent->second);
			setObject.emplace_back(ourCurrent->first);
			setObject.emplace_back(ourObjects[ourCurrent->first]);

			if (setChecksum.size() == 100u) {
				flushSets();
			}
		});

		auto delOne ([&]() {
			delChecksum.emplace_back(redisCurrent->first);
			delObject.emplace_back(redisCurrent->first);

			if (delChecksum.size() == 100u) {
				flushDels();
			}
		});

		for (;;) {
			if (redisCurrent == redisEnd) {
				for (; ourCurrent != ourEnd; ++ourCurrent) {
					setOne();
				}

				break;
			} else if (ourCurrent == ourEnd) {
				for (; redisCurrent != redisEnd; ++redisCurrent) {
					delOne();
				}

				break;
			} else if (redisCurrent->first < ourCurrent->first) {
				delOne();
				++redisCurrent;
			} else if (redisCurrent->first > ourCurrent->first) {
				setOne();
				++ourCurrent;
			} else {
				if (redisCurrent->second != ourCurrent->second) {
					setOne();
				}

				++redisCurrent;
				++ourCurrent;
			}
		}

		if (delChecksum.size()) {
			flushDels();
		}

		if (setChecksum.size()) {
			flushSets();
		}

		for (auto& key : GetTypeDumpSignalKeys(type)) {
			rcon->FireAndForgetQuery({"XADD", "icinga:dump", "*", "key", key, "state", "done"}, Prio::Config);
		}
		rcon->Sync();
	});

	upq.Join();

	if (upq.HasExceptions()) {
		for (boost::exception_ptr exc : upq.GetExceptions()) {
			try {
				if (exc) {
					boost::rethrow_exception(exc);
			}
			} catch(const std::exception& e) {
				Log(LogCritical, "IcingaDB")
						<< "Exception during ConfigDump: " << e.what();
			}
		}
	}

	for (auto& key : globalKeys) {
		m_Rcon->FireAndForgetQuery({"XADD", "icinga:dump", "*", "key", key, "state", "done"}, Prio::Config);
	}

	m_Rcon->FireAndForgetQuery({"XADD", "icinga:dump", "*", "key", "*", "state", "done"}, Prio::Config);

	// enqueue a callback that will notify us once all previous queries were executed and wait for this event
	std::promise<void> p;
	m_Rcon->EnqueueCallback([&p](boost::asio::yield_context& yc) { p.set_value(); }, Prio::Config);
	p.get_future().wait();

	auto endTime (Utility::GetTime());
	auto took (endTime - startTime);

	SetLastdumpTook(took);
	SetLastdumpEnd(endTime);

	Log(LogInformation, "IcingaDB")
		<< "Initial config/status dump finished in " << took << " seconds.";
}

std::vector<std::vector<intrusive_ptr<ConfigObject>>> IcingaDB::ChunkObjects(std::vector<intrusive_ptr<ConfigObject>> objects, size_t chunkSize) {
	std::vector<std::vector<intrusive_ptr<ConfigObject>>> chunks;
	auto offset (objects.begin());
	auto end (objects.end());

	chunks.reserve((std::distance(offset, end) + chunkSize - 1) / chunkSize);

	while (std::distance(offset, end) >= chunkSize) {
		auto until (offset + chunkSize);
		chunks.emplace_back(offset, until);
		offset = until;
	}

	if (offset != end) {
		chunks.emplace_back(offset, end);
	}

	return chunks;
}

void IcingaDB::DeleteKeys(const RedisConnection::Ptr& conn, const std::vector<String>& keys, RedisConnection::QueryPriority priority) {
	std::vector<String> query = {"DEL"};
	for (auto& key : keys) {
		query.emplace_back(key);
	}

	conn->FireAndForgetQuery(std::move(query), priority);
}

std::vector<String> IcingaDB::GetTypeOverwriteKeys(const String& type)
{
	std::vector<String> keys = {
			m_PrefixConfigObject + type + ":customvar",
	};

	if (type == "host" || type == "service" || type == "user") {
		keys.emplace_back(m_PrefixConfigObject + type + "group:member");
		keys.emplace_back(m_PrefixConfigObject + type + ":state");
		keys.emplace_back(m_PrefixConfigCheckSum + type + ":state");
	} else if (type == "timeperiod") {
		keys.emplace_back(m_PrefixConfigObject + type + ":override:include");
		keys.emplace_back(m_PrefixConfigObject + type + ":override:exclude");
		keys.emplace_back(m_PrefixConfigObject + type + ":range");
	} else if (type == "notification") {
		keys.emplace_back(m_PrefixConfigObject + type + ":user");
		keys.emplace_back(m_PrefixConfigObject + type + ":usergroup");
		keys.emplace_back(m_PrefixConfigObject + type + ":recipient");
	} else if (type == "checkcommand" || type == "notificationcommand" || type == "eventcommand") {
		keys.emplace_back(m_PrefixConfigObject + type + ":envvar");
		keys.emplace_back(m_PrefixConfigCheckSum + type + ":envvar");
		keys.emplace_back(m_PrefixConfigObject + type + ":argument");
		keys.emplace_back(m_PrefixConfigCheckSum + type + ":argument");
	}

	return keys;
}

std::vector<String> IcingaDB::GetTypeDumpSignalKeys(const Type::Ptr& type)
{
	String lcType = type->GetName().ToLower();
	std::vector<String> keys = {m_PrefixConfigObject + lcType};

	if (CustomVarObject::TypeInstance->IsAssignableFrom(type)) {
		keys.emplace_back(m_PrefixConfigObject + lcType + ":customvar");
	}

	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		keys.emplace_back(m_PrefixConfigObject + lcType + "group:member");
		keys.emplace_back(m_PrefixConfigObject + lcType + ":state");
	} else if (type == User::TypeInstance) {
		keys.emplace_back(m_PrefixConfigObject + lcType + "group:member");
	} else if (type == TimePeriod::TypeInstance) {
		keys.emplace_back(m_PrefixConfigObject + lcType + ":override:include");
		keys.emplace_back(m_PrefixConfigObject + lcType + ":override:exclude");
		keys.emplace_back(m_PrefixConfigObject + lcType + ":range");
	} else if (type == Notification::TypeInstance) {
		keys.emplace_back(m_PrefixConfigObject + lcType + ":user");
		keys.emplace_back(m_PrefixConfigObject + lcType + ":usergroup");
		keys.emplace_back(m_PrefixConfigObject + lcType + ":recipient");
	} else if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance) {
		keys.emplace_back(m_PrefixConfigObject + lcType + ":envvar");
		keys.emplace_back(m_PrefixConfigObject + lcType + ":argument");
	}

	return keys;
}

template<typename ConfigType>
static ConfigObject::Ptr GetObjectByName(const String& name)
{
	return ConfigObject::GetObject<ConfigType>(name);
}

void IcingaDB::InsertObjectDependencies(const ConfigObject::Ptr& object, const String typeName, std::map<String, std::vector<String>>& hMSets,
		std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate)
{
	String objectKey = GetObjectIdentifier(object);
	String objectKeyName = typeName + "_id";

	Type::Ptr type = object->GetReflectionType();

	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);

	if (customVarObject) {
		auto vars(SerializeVars(customVarObject->GetVars()));
		if (vars) {
			auto& typeCvs (hMSets[m_PrefixConfigObject + typeName + ":customvar"]);
			auto& allCvs (hMSets[m_PrefixConfigObject + "customvar"]);

			ObjectLock varsLock(vars);
			Array::Ptr varsArray(new Array);

			varsArray->Reserve(vars->GetLength());

			for (auto& kv : vars) {
				if (runtimeUpdate || m_DumpedGlobals.CustomVar.IsNew(kv.first)) {
					allCvs.emplace_back(kv.first);
					allCvs.emplace_back(JsonEncode(kv.second));

					if (runtimeUpdate) {
						AddObjectDataToRuntimeUpdates(runtimeUpdates, kv.first, m_PrefixConfigObject + "customvar", kv.second);
					}
				}

				String id = HashValue(new Array({m_EnvironmentId, kv.first, object->GetName()}));
				typeCvs.emplace_back(id);

				Dictionary::Ptr	data = new Dictionary({{objectKeyName, objectKey}, {"environment_id", m_EnvironmentId}, {"customvar_id", kv.first}});
				typeCvs.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":customvar", data);
				}
			}
		}
	}

	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

		String actionUrl = checkable->GetActionUrl();
		String notesUrl = checkable->GetNotesUrl();
		String iconImage = checkable->GetIconImage();
		if (!actionUrl.IsEmpty()) {
			auto& actionUrls (hMSets[m_PrefixConfigObject + "action:url"]);

			auto id (HashValue(new Array({m_EnvironmentId, actionUrl})));

			if (runtimeUpdate || m_DumpedGlobals.ActionUrl.IsNew(id)) {
				actionUrls.emplace_back(std::move(id));
				Dictionary::Ptr data = new Dictionary({{"environment_id", m_EnvironmentId}, {"action_url", actionUrl}});
				actionUrls.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, actionUrls.at(actionUrls.size() - 2u), m_PrefixConfigObject + "action:url", data);
				}
			}
		}
		if (!notesUrl.IsEmpty()) {
			auto& notesUrls (hMSets[m_PrefixConfigObject + "notes:url"]);

			auto id (HashValue(new Array({m_EnvironmentId, notesUrl})));

			if (runtimeUpdate || m_DumpedGlobals.NotesUrl.IsNew(id)) {
				notesUrls.emplace_back(std::move(id));
				Dictionary::Ptr data = new Dictionary({{"environment_id", m_EnvironmentId}, {"notes_url", notesUrl}});
				notesUrls.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, notesUrls.at(notesUrls.size() - 2u), m_PrefixConfigObject + "notes:url", data);
				}
			}
		}
		if (!iconImage.IsEmpty()) {
			auto& iconImages (hMSets[m_PrefixConfigObject + "icon:image"]);

			auto id (HashValue(new Array({m_EnvironmentId, iconImage})));

			if (runtimeUpdate || m_DumpedGlobals.IconImage.IsNew(id)) {
				iconImages.emplace_back(std::move(id));
				Dictionary::Ptr data = new Dictionary({{"environment_id", m_EnvironmentId}, {"icon_image", iconImage}});
				iconImages.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, iconImages.at(iconImages.size() - 2u), m_PrefixConfigObject + "icon:image", data);
				}
			}
		}

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		ConfigObject::Ptr (*getGroup)(const String& name);
		Array::Ptr groups;
		if (service) {
			groups = service->GetGroups();
			getGroup = &::GetObjectByName<ServiceGroup>;
		} else {
			groups = host->GetGroups();
			getGroup = &::GetObjectByName<HostGroup>;
		}

		if (groups) {
			ObjectLock groupsLock(groups);
			Array::Ptr groupIds(new Array);

			groupIds->Reserve(groups->GetLength());

			auto& members (hMSets[m_PrefixConfigObject + typeName + "group:member"]);

			for (auto& group : groups) {
				auto groupObj ((*getGroup)(group));
				String groupId = GetObjectIdentifier(groupObj);
				String id = HashValue(new Array({m_EnvironmentId, groupObj->GetName(), object->GetName()}));
				members.emplace_back(id);
				Dictionary::Ptr data = new Dictionary({{objectKeyName, objectKey}, {"environment_id", m_EnvironmentId}, {typeName + "group_id", groupId}});
				members.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + "group:member", data);
				}

				groupIds->Add(groupId);
			}
		}

		InsertCheckableDependencies(checkable, hMSets, runtimeUpdate ? &runtimeUpdates : nullptr);

		return;
	}

	if (type == TimePeriod::TypeInstance) {
		TimePeriod::Ptr timeperiod = static_pointer_cast<TimePeriod>(object);

		Dictionary::Ptr ranges = timeperiod->GetRanges();
		if (ranges) {
			ObjectLock rangesLock(ranges);
			Array::Ptr rangeIds(new Array);
			auto& typeRanges (hMSets[m_PrefixConfigObject + typeName + ":range"]);

			rangeIds->Reserve(ranges->GetLength());

			for (auto& kv : ranges) {
				String rangeId = HashValue(new Array({m_EnvironmentId, kv.first, kv.second}));
				rangeIds->Add(rangeId);

				String id = HashValue(new Array({m_EnvironmentId, kv.first, kv.second, object->GetName()}));
				typeRanges.emplace_back(id);
				Dictionary::Ptr data = new Dictionary({{"environment_id", m_EnvironmentId}, {"timeperiod_id", objectKey}, {"range_key", kv.first}, {"range_value", kv.second}});
				typeRanges.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":range", data);
				}
			}
		}

		Array::Ptr includes;
		ConfigObject::Ptr (*getInclude)(const String& name);
		includes = timeperiod->GetIncludes();
		getInclude = &::GetObjectByName<TimePeriod>;

		Array::Ptr includeChecksums = new Array();

		ObjectLock includesLock(includes);
		ObjectLock includeChecksumsLock(includeChecksums);

		includeChecksums->Reserve(includes->GetLength());


		auto& includs (hMSets[m_PrefixConfigObject + typeName + ":override:include"]);
		for (auto include : includes) {
			auto includeTp ((*getInclude)(include.Get<String>()));
			String includeId = GetObjectIdentifier(includeTp);
			includeChecksums->Add(includeId);

			String id = HashValue(new Array({m_EnvironmentId, includeTp->GetName(), object->GetName()}));
			includs.emplace_back(id);
			Dictionary::Ptr data = new Dictionary({{"environment_id", m_EnvironmentId}, {"timeperiod_id", objectKey}, {"include_id", includeId}});
			includs.emplace_back(JsonEncode(data));

			if (runtimeUpdate) {
				AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":override:include", data);
			}
		}

		Array::Ptr excludes;
		ConfigObject::Ptr (*getExclude)(const String& name);

		excludes = timeperiod->GetExcludes();
		getExclude = &::GetObjectByName<TimePeriod>;

		Array::Ptr excludeChecksums = new Array();

		ObjectLock excludesLock(excludes);
		ObjectLock excludeChecksumsLock(excludeChecksums);

		excludeChecksums->Reserve(excludes->GetLength());

		auto& excluds (hMSets[m_PrefixConfigObject + typeName + ":override:exclude"]);

		for (auto exclude : excludes) {
			auto excludeTp ((*getExclude)(exclude.Get<String>()));
			String excludeId = GetObjectIdentifier(excludeTp);
			excludeChecksums->Add(excludeId);

			String id = HashValue(new Array({m_EnvironmentId, excludeTp->GetName(), object->GetName()}));
			excluds.emplace_back(id);
			Dictionary::Ptr data = new Dictionary({{"environment_id", m_EnvironmentId}, {"timeperiod_id", objectKey}, {"exclude_id", excludeId}});
			excluds.emplace_back(JsonEncode(data));

			if (runtimeUpdate) {
				AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":override:exclude", data);
			}
		}

		return;
	}

	if (type == User::TypeInstance) {
		User::Ptr user = static_pointer_cast<User>(object);
		Array::Ptr groups = user->GetGroups();

		if (groups) {
			ObjectLock groupsLock(groups);
			Array::Ptr groupIds(new Array);

			groupIds->Reserve(groups->GetLength());

			auto& members (hMSets[m_PrefixConfigObject + typeName + "group:member"]);
			auto& notificationRecipients (hMSets[m_PrefixConfigObject + "notification:recipient"]);

			for (auto& group : groups) {
				UserGroup::Ptr groupObj = UserGroup::GetByName(group);
				String groupId = GetObjectIdentifier(groupObj);
				String id = HashValue(new Array({m_EnvironmentId, groupObj->GetName(), object->GetName()}));
				members.emplace_back(id);
				Dictionary::Ptr data = new Dictionary({{"user_id", objectKey}, {"environment_id", m_EnvironmentId}, {"usergroup_id", groupId}});
				members.emplace_back(JsonEncode(data));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + "group:member", data);

					// Recipients are handled by notifications during initial dumps and only need to be handled here during runtime (e.g. User creation).
					for (auto& notification : groupObj->GetNotifications()) {
						String recipientId = HashValue(new Array({m_EnvironmentId, "usergroupuser", user->GetName(), groupObj->GetName(), notification->GetName()}));
						notificationRecipients.emplace_back(recipientId);
						Dictionary::Ptr recipientData = new Dictionary({{"notification_id", GetObjectIdentifier(notification)}, {"environment_id", m_EnvironmentId}, {"user_id", objectKey}, {"usergroup_id", groupId}});
						notificationRecipients.emplace_back(JsonEncode(recipientData));

						AddObjectDataToRuntimeUpdates(runtimeUpdates, recipientId, m_PrefixConfigObject + "notification:recipient", recipientData);
					}
				}

				groupIds->Add(groupId);
			}
		}

		return;
	}

	if (type == Notification::TypeInstance) {
		Notification::Ptr notification = static_pointer_cast<Notification>(object);

		std::set<User::Ptr> users = notification->GetUsers();
		Array::Ptr userIds = new Array();

		auto usergroups(notification->GetUserGroups());
		Array::Ptr usergroupIds = new Array();

		userIds->Reserve(users.size());

		auto& usrs (hMSets[m_PrefixConfigObject + typeName + ":user"]);
		auto& notificationRecipients (hMSets[m_PrefixConfigObject + typeName + ":recipient"]);

		for (auto& user : users) {
			String userId = GetObjectIdentifier(user);
			String id = HashValue(new Array({m_EnvironmentId, "user", user->GetName(), object->GetName()}));
			usrs.emplace_back(id);
			notificationRecipients.emplace_back(id);

			Dictionary::Ptr data = new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"user_id", userId}});
			String dataJson = JsonEncode(data);
			usrs.emplace_back(dataJson);
			notificationRecipients.emplace_back(dataJson);

			if (runtimeUpdate) {
				AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":user", data);
				AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":recipient", data);
			}

			userIds->Add(userId);
		}

		usergroupIds->Reserve(usergroups.size());

		auto& groups (hMSets[m_PrefixConfigObject + typeName + ":usergroup"]);

		for (auto& usergroup : usergroups) {
			String usergroupId = GetObjectIdentifier(usergroup);
			String id = HashValue(new Array({m_EnvironmentId, "usergroup", usergroup->GetName(), object->GetName()}));
			groups.emplace_back(id);
			notificationRecipients.emplace_back(id);

			Dictionary::Ptr groupData = new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"usergroup_id", usergroupId}});
			String groupDataJson = JsonEncode(groupData);
			groups.emplace_back(groupDataJson);
			notificationRecipients.emplace_back(groupDataJson);

			if (runtimeUpdate) {
				AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":usergroup", groupData);
				AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":recipient", groupData);
			}

			for (const User::Ptr& user : usergroup->GetMembers()) {
				String userId = GetObjectIdentifier(user);
				String recipientId = HashValue(new Array({m_EnvironmentId, "usergroupuser", user->GetName(), usergroup->GetName(), notification->GetName()}));
				notificationRecipients.emplace_back(recipientId);
				Dictionary::Ptr userData = new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"user_id", userId}, {"usergroup_id", usergroupId}});
				notificationRecipients.emplace_back(JsonEncode(userData));

				if (runtimeUpdate) {
					AddObjectDataToRuntimeUpdates(runtimeUpdates, recipientId, m_PrefixConfigObject + typeName + ":recipient", userData);
				}
			}

			usergroupIds->Add(usergroupId);
		}

		return;
	}

	if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance) {
		Command::Ptr command = static_pointer_cast<Command>(object);

		Dictionary::Ptr arguments = command->GetArguments();
		if (arguments) {
			ObjectLock argumentsLock(arguments);
			auto& typeArgs (hMSets[m_PrefixConfigObject + typeName + ":argument"]);
			auto& argChksms (hMSets[m_PrefixConfigCheckSum + typeName + ":argument"]);

			for (auto& kv : arguments) {
				Dictionary::Ptr values;
				if (kv.second.IsObjectType<Dictionary>()) {
					values = kv.second;
					values = values->ShallowClone();
				} else if (kv.second.IsObjectType<Array>()) {
					values = new Dictionary({{"value", JsonEncode(kv.second)}});
				} else {
					values = new Dictionary({{"value", kv.second}});
				}

				for (const char *attr : {"value", "set_if", "separator"}) {
					Value value;

					// Stringify if set.
					if (values->Get(attr, &value)) {
						switch (value.GetType()) {
							case ValueEmpty:
							case ValueString:
								break;
							case ValueObject:
								values->Set(attr, value.Get<Object::Ptr>()->ToString());
								break;
							default:
								values->Set(attr, JsonEncode(value));
						}
					}
				}

				for (const char *attr : {"repeat_key", "required", "skip_key"}) {
					Value value;

					// Boolify if set.
					if (values->Get(attr, &value)) {
						values->Set(attr, value.ToBool());
					}
				}

				{
					Value order;

					// Intify if set.
					if (values->Get("order", &order)) {
						values->Set("order", (int)order);
					}
				}

				values->Set(objectKeyName, objectKey);
				values->Set("argument_key", kv.first);
				values->Set("environment_id", m_EnvironmentId);

				String id = HashValue(new Array({m_EnvironmentId, kv.first, object->GetName()}));

				typeArgs.emplace_back(id);
				typeArgs.emplace_back(JsonEncode(values));

				argChksms.emplace_back(id);
				String checksum = HashValue(kv.second);
				argChksms.emplace_back(JsonEncode(new Dictionary({{"checksum", checksum}})));

				if (runtimeUpdate) {
					values->Set("checksum", checksum);
					AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":argument", values);
				}
			}
		}

		Dictionary::Ptr envvars = command->GetEnv();
		if (envvars) {
			ObjectLock envvarsLock(envvars);
			Array::Ptr envvarIds(new Array);
			auto& typeVars (hMSets[m_PrefixConfigObject + typeName + ":envvar"]);
			auto& varChksms (hMSets[m_PrefixConfigCheckSum + typeName + ":envvar"]);

			envvarIds->Reserve(envvars->GetLength());

			for (auto& kv : envvars) {
				Dictionary::Ptr values;
				if (kv.second.IsObjectType<Dictionary>()) {
					values = kv.second;
					values = values->ShallowClone();
				} else if (kv.second.IsObjectType<Array>()) {
					values = new Dictionary({{"value", JsonEncode(kv.second)}});
				} else {
					values = new Dictionary({{"value", kv.second}});
				}

				{
					Value value;

					// JsonEncode() the value if it's set.
					if (values->Get("value", &value)) {
						values->Set("value", JsonEncode(value));
					}
				}

				values->Set(objectKeyName, objectKey);
				values->Set("envvar_key", kv.first);
				values->Set("environment_id", m_EnvironmentId);

				String id = HashValue(new Array({m_EnvironmentId, kv.first, object->GetName()}));

				typeVars.emplace_back(id);
				typeVars.emplace_back(JsonEncode(values));

				varChksms.emplace_back(id);
				String checksum = HashValue(kv.second);
				varChksms.emplace_back(JsonEncode(new Dictionary({{"checksum", checksum}})));

				if (runtimeUpdate) {
					values->Set("checksum", checksum);
					AddObjectDataToRuntimeUpdates(runtimeUpdates, id, m_PrefixConfigObject + typeName + ":envvar", values);
				}
			}
		}

		return;
	}
}

/**
 * Inserts the dependency data for a Checkable object into the given Redis HMSETs and runtime updates.
 *
 * This function is responsible for serializing the in memory representation Checkable dependencies into
 * Redis HMSETs and runtime updates (if any) according to the Icinga DB schema. The serialized data consists
 * of the following Redis HMSETs:
 * - RedisKey::DependencyNode: Contains dependency node data representing each host, service, and redundancy group
 *   in the dependency graph.
 * - RedisKey::DependencyEdge: Dependency edge information representing all connections between the nodes.
 * - RedisKey::RedundancyGroup: Redundancy group data representing all redundancy groups in the graph.
 * - RedisKey::RedundancyGroupState: State information for redundancy groups.
 * - RedisKey::DependencyEdgeState: State information for (each) dependency edge. Multiple edges may share the
 *	 same state.
 *
 * If the `onlyDependencyGroup` parameter is set, only dependencies from this group are processed. This is useful
 * when only a specific dependency group should be processed, e.g. during runtime updates. For initial config dumps,
 * it shouldn't be necessary to set the `runtimeUpdates` and `onlyDependencyGroup` parameters.
 *
 * @param checkable The checkable object to extract dependencies from.
 * @param hMSets The map of Redis HMSETs to insert the dependency data into.
 * @param runtimeUpdates If set, runtime updates are additionally added to this vector.
 * @param onlyDependencyGroup If set, only process dependency objects from this group.
 */
void IcingaDB::InsertCheckableDependencies(
	const Checkable::Ptr& checkable,
	std::map<String, RedisConnection::Query>& hMSets,
	std::vector<Dictionary::Ptr>* runtimeUpdates,
	const DependencyGroup::Ptr& onlyDependencyGroup
)
{
	// Only generate a dependency node event if the Checkable is actually part of some dependency graph.
	// That's, it either depends on other Checkables or others depend on it, and in both cases, we have
	// to at least generate a dependency node entry for it.
	if (!checkable->HasAnyDependencies()) {
		return;
	}

	// First and foremost, generate a dependency node entry for the provided Checkable object and insert it into
	// the HMSETs map and if set, the `runtimeUpdates` vector.
	auto [host, service] = GetHostService(checkable);
	auto checkableId(GetObjectIdentifier(checkable));
	{
		Dictionary::Ptr data(new Dictionary{{"environment_id", m_EnvironmentId}, {"host_id", GetObjectIdentifier(host)}});
		if (service) {
			data->Set("service_id", checkableId);
		}

		AddDataToHmSets(hMSets, RedisKey::DependencyNode, checkableId, data);
		if (runtimeUpdates) {
			AddObjectDataToRuntimeUpdates(*runtimeUpdates, checkableId, m_PrefixConfigObject + "dependency:node", data);
		}
	}

	// If `onlyDependencyGroup` is provided, process the dependencies only from that group; otherwise,
	// retrieve all the dependency groups that the Checkable object is part of.
	std::vector<DependencyGroup::Ptr> dependencyGroups{onlyDependencyGroup};
	if (!onlyDependencyGroup) {
		dependencyGroups = checkable->GetDependencyGroups();
	}

	for (auto& dependencyGroup : dependencyGroups) {
		String edgeFromNodeId(checkableId);
		bool syncSharedEdgeState(false);

		if (!dependencyGroup->IsRedundancyGroup()) {
			// Non-redundant dependency groups are just placeholders and never get synced to Redis, thus just figure
			// out whether we have to sync the shared edge state. For runtime updates the states are sent via the
			// UpdateDependenciesState() method, thus we don't have to sync them here.
			syncSharedEdgeState = !runtimeUpdates && m_DumpedGlobals.DependencyGroup.IsNew(dependencyGroup->GetCompositeKey());
		} else {
			auto redundancyGroupId(HashValue(new Array{m_EnvironmentId, dependencyGroup->GetCompositeKey()}));
			dependencyGroup->SetIcingaDBIdentifier(redundancyGroupId);

			edgeFromNodeId = redundancyGroupId;

			// During the initial config sync, multiple children can depend on the same redundancy group, sync it only
			// the first time it is encountered. Though, if this is a runtime update, we have to re-serialize and sync
			// the redundancy group unconditionally, as we don't know whether it was already synced or the context that
			// triggered this update.
			if (runtimeUpdates || m_DumpedGlobals.DependencyGroup.IsNew(redundancyGroupId)) {
				Dictionary::Ptr groupData(new Dictionary{
					{"environment_id", m_EnvironmentId},
					{"display_name", dependencyGroup->GetRedundancyGroupName()},
				});
				// Set/refresh the redundancy group data in the Redis HMSETs (redundancy_group database table).
				AddDataToHmSets(hMSets, RedisKey::RedundancyGroup, redundancyGroupId, groupData);

				Dictionary::Ptr nodeData(new Dictionary{
					{"environment_id", m_EnvironmentId},
					{"redundancy_group_id", redundancyGroupId},
				});
				// Obviously, the redundancy group is part of some dependency chain, thus we have to generate
				// dependency node entry for it as well.
				AddDataToHmSets(hMSets, RedisKey::DependencyNode, redundancyGroupId, nodeData);

				if (runtimeUpdates) {
					// Send the same data sent to the Redis HMSETs to the runtime updates stream as well.
					AddObjectDataToRuntimeUpdates(*runtimeUpdates, redundancyGroupId, m_PrefixConfigObject + "redundancygroup", groupData);
					AddObjectDataToRuntimeUpdates(*runtimeUpdates, redundancyGroupId, m_PrefixConfigObject + "dependency:node", nodeData);
				} else {
					syncSharedEdgeState = true;

					// Serialize and sync the redundancy group state information a) to the RedundancyGroupState and b)
					// to the DependencyEdgeState HMSETs. The latter is shared by all child Checkables of the current
					// redundancy group, and since they all depend on the redundancy group, the state of that group is
					// basically the state of the dependency edges between the children and the redundancy group.
					auto stateAttrs(SerializeRedundancyGroupState(checkable, dependencyGroup));
					AddDataToHmSets(hMSets, RedisKey::RedundancyGroupState, redundancyGroupId, stateAttrs);
					AddDataToHmSets(hMSets, RedisKey::DependencyEdgeState, redundancyGroupId, Dictionary::Ptr(new Dictionary{
						{"id", redundancyGroupId},
						{"environment_id", m_EnvironmentId},
						{"failed", stateAttrs->Get("failed")},
					}));
				}
			}

			Dictionary::Ptr data(new Dictionary{
				{"environment_id", m_EnvironmentId},
				{"from_node_id", checkableId},
				{"to_node_id", redundancyGroupId},
				// All redundancy group members share the same state, thus use the group ID as a reference.
				{"dependency_edge_state_id", redundancyGroupId},
				{"display_name", dependencyGroup->GetRedundancyGroupName()},
			});

			// Generate a dependency edge entry representing the connection between the Checkable and the redundancy
			// group. This Checkable dependes on the redundancy group (is a child of it), thus the "dependency_edge_state_id"
			// is set to the redundancy group ID. Note that if this group has multiple children, they all will have the
			// same "dependency_edge_state_id" value.
			auto edgeId(HashValue(new Array{checkableId, redundancyGroupId}));
			AddDataToHmSets(hMSets, RedisKey::DependencyEdge, edgeId, data);

			if (runtimeUpdates) {
				AddObjectDataToRuntimeUpdates(*runtimeUpdates, edgeId, m_PrefixConfigObject + "dependency:edge", data);
			}
		}

		auto dependencies(dependencyGroup->GetDependenciesForChild(checkable.get()));
		// Sort the dependencies by their parent Checkable object to ensure that all dependency objects that share the
		// same parent Checkable are placed next to each other in the container. See the while loop below for more info!
		std::sort(dependencies.begin(), dependencies.end(), [](const Dependency::Ptr& lhs, const Dependency::Ptr& rhs) {
			return lhs->GetParent() < rhs->GetParent();
		});

		// Traverse through each dependency objects within the current dependency group the provided Checkable depend
		// on and generate a dependency edge entry. The generated dependency edge "from_node_id" may vary depending on
		// whether the dependency group is a redundancy group or not. If it's a redundancy group, the "from_node_id"
		// will be the redundancy group ID; otherwise, it will be the current Checkable ID. However, the "to_node_id"
		// value will always be the parent Checkable ID of the dependency object.
		for (auto it(dependencies.begin()); it != dependencies.end(); /* no increment */) {
			auto dependency(*it);
			auto parent(dependency->GetParent());
			auto displayName(dependency->GetShortName());

			Dictionary::Ptr edgeStateAttrs(SerializeDependencyEdgeState(dependencyGroup, dependency));

			// In case there are multiple Dependency objects with the same parent, these are merged into a single edge
			// to prevent duplicate edges in the resulting graph. All objects with the same parent were placed next to
			// each other by the sort function above.
			//
			// Additionally, the iterator for the surrounding loop is incremented by this loop: after it has finished,
			// "it" will either point to the next dependency with a different parent or to the end of the container.
			while (++it != dependencies.end() && (*it)->GetParent() == parent) {
				displayName += ", " + (*it)->GetShortName();
				if (syncSharedEdgeState && edgeStateAttrs->Get("failed") == false) {
					edgeStateAttrs = SerializeDependencyEdgeState(dependencyGroup, *it);
				}
			}

			Dictionary::Ptr data(new Dictionary{
				{"environment_id", m_EnvironmentId},
				{"from_node_id", edgeFromNodeId},
				{"to_node_id", GetObjectIdentifier(parent)},
				{"dependency_edge_state_id", edgeStateAttrs->Get("id")},
				{"display_name", std::move(displayName)},
			});

			auto edgeId(HashValue(new Array{data->Get("from_node_id"), data->Get("to_node_id")}));
			AddDataToHmSets(hMSets, RedisKey::DependencyEdge, edgeId, data);

			if (runtimeUpdates) {
				AddObjectDataToRuntimeUpdates(*runtimeUpdates, edgeId, m_PrefixConfigObject + "dependency:edge", data);
			} else if (syncSharedEdgeState) {
				AddDataToHmSets(hMSets, RedisKey::DependencyEdgeState, edgeStateAttrs->Get("id"), edgeStateAttrs);
			}
		}
	}
}

/**
 * Update the state information of a checkable in Redis.
 *
 * What is updated exactly depends on the mode parameter:
 *  - Volatile: Update the volatile state information stored in icinga:host:state or icinga:service:state as well as
 *    the corresponding checksum stored in icinga:checksum:host:state or icinga:checksum:service:state.
 *  - RuntimeOnly: Write a runtime update to the icinga:runtime:state stream. It is up to the caller to ensure that
 *    identical volatile state information was already written before to avoid inconsistencies. This mode is only
 *    useful to upgrade a previous Volatile to a Full operation, otherwise Full should be used.
 *  - Full: Perform an update of all state information in Redis, that is updating the volatile information and sending
 *    a corresponding runtime update so that this state update gets written through to the persistent database by a
 *    running icingadb process.
 *
 * @param checkable State of this checkable is updated in Redis
 * @param mode Mode of operation (StateUpdate::Volatile, StateUpdate::RuntimeOnly, or StateUpdate::Full)
 */
void IcingaDB::UpdateState(const Checkable::Ptr& checkable, StateUpdate mode)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String objectType = GetLowerCaseTypeNameDB(checkable);
	String objectKey = GetObjectIdentifier(checkable);

	Dictionary::Ptr stateAttrs = SerializeState(checkable);

	String redisStateKey = m_PrefixConfigObject + objectType + ":state";
	String redisChecksumKey = m_PrefixConfigCheckSum + objectType + ":state";
	String checksum = HashValue(stateAttrs);

	if (mode & StateUpdate::Volatile) {
		m_Rcon->FireAndForgetQueries({
			{"HSET", redisStateKey, objectKey, JsonEncode(stateAttrs)},
			{"HSET", redisChecksumKey, objectKey, JsonEncode(new Dictionary({{"checksum", checksum}}))},
		}, Prio::RuntimeStateSync);
	}

	if (mode & StateUpdate::RuntimeOnly) {
		ObjectLock olock(stateAttrs);

		std::vector<String> streamadd({
			"XADD", "icinga:runtime:state", "MAXLEN", "~", "1000000", "*",
			"runtime_type", "upsert",
			"redis_key", redisStateKey,
			"checksum", checksum,
		});

		for (const Dictionary::Pair& kv : stateAttrs) {
			streamadd.emplace_back(kv.first);
			streamadd.emplace_back(IcingaToStreamValue(kv.second));
		}

		m_Rcon->FireAndForgetQuery(std::move(streamadd), Prio::RuntimeStateStream, {0, 1});
	}
}

/**
 * Send dependencies state information of the given Checkable to Redis.
 *
 * If the dependencyGroup parameter is set, only the dependencies state of that group are sent. Otherwise, all
 * dependency groups of the provided Checkable are processed.
 *
 * @param checkable The Checkable you want to send the dependencies state update for
 * @param onlyDependencyGroup If set, send state updates only for this dependency group and its dependencies.
 * @param seenGroups A container to track already processed DependencyGroups to avoid duplicate state updates.
 */
void IcingaDB::UpdateDependenciesState(const Checkable::Ptr& checkable, const DependencyGroup::Ptr& onlyDependencyGroup,
	std::set<DependencyGroup*>* seenGroups) const
{
	if (!m_Rcon || !m_Rcon->IsConnected()) {
		return;
	}

	std::vector<DependencyGroup::Ptr> dependencyGroups{onlyDependencyGroup};
	if (!onlyDependencyGroup) {
		dependencyGroups = checkable->GetDependencyGroups();
		if (dependencyGroups.empty()) {
			return;
		}
	}

	RedisConnection::Queries streamStates;
	auto addDependencyStateToStream([this, &streamStates](const String& redisKey, const Dictionary::Ptr& stateAttrs) {
		RedisConnection::Query xAdd{
			"XADD", "icinga:runtime:state", "MAXLEN", "~", "1000000", "*", "runtime_type", "upsert",
			"redis_key", redisKey
		};
		ObjectLock olock(stateAttrs);
		for (auto& [key, value] : stateAttrs) {
			xAdd.emplace_back(key);
			xAdd.emplace_back(IcingaToStreamValue(value));
		}
		streamStates.emplace_back(std::move(xAdd));
	});

	std::map<String, RedisConnection::Query> hMSets;
	for (auto& dependencyGroup : dependencyGroups) {
		bool isRedundancyGroup(dependencyGroup->IsRedundancyGroup());
		if (isRedundancyGroup && dependencyGroup->GetIcingaDBIdentifier().IsEmpty()) {
			// Way too soon! The Icinga DB hash will be set during the initial config dump, but this state
			// update seems to occur way too early. So, we've to skip it for now and wait for the next one.
			// The m_ConfigDumpInProgress flag is probably still set to true at this point!
			continue;
		}

		if (seenGroups && !seenGroups->insert(dependencyGroup.get()).second) {
			// Usually, if the seenGroups set is provided, IcingaDB is triggering a runtime state update for ALL
			// children of a given initiator Checkable (parent). In such cases, we may end up with lots of useless
			// state updates as all the children of a non-redundant group a) share the same entry in the database b)
			// it doesn't matter which child triggers the state update first all the subsequent updates are just useless.
			//
			// Likewise, for redundancy groups, all children of a redundancy group share the same set of parents
			// and thus the resulting state information would be the same from each child Checkable perspective.
			// So, serializing the redundancy group state information only once is sufficient.
			continue;
		}

		auto dependencies(dependencyGroup->GetDependenciesForChild(checkable.get()));
		std::sort(dependencies.begin(), dependencies.end(), [](const Dependency::Ptr& lhs, const Dependency::Ptr& rhs) {
			return lhs->GetParent() < rhs->GetParent();
		});
		for (auto it(dependencies.begin()); it != dependencies.end(); /* no increment */) {
			const auto& dependency(*it);

			Dictionary::Ptr stateAttrs;
			// Note: The following loop is intended to cover some possible special cases but may not occur in practice
			// that often. That is, having two or more dependency objects that point to the same parent Checkable.
			// So, traverse all those duplicates and merge their relevant state information into a single edge.
			for (; it != dependencies.end() && (*it)->GetParent() == dependency->GetParent(); ++it) {
				if (!stateAttrs || stateAttrs->Get("failed") == false) {
					stateAttrs = SerializeDependencyEdgeState(dependencyGroup, *it);
				}
			}

			addDependencyStateToStream(m_PrefixConfigObject + "dependency:edge:state", stateAttrs);
			AddDataToHmSets(hMSets, RedisKey::DependencyEdgeState, stateAttrs->Get("id"), stateAttrs);
		}

		if (isRedundancyGroup) {
			Dictionary::Ptr stateAttrs(SerializeRedundancyGroupState(checkable, dependencyGroup));

			Dictionary::Ptr sharedGroupState(stateAttrs->ShallowClone());
			sharedGroupState->Remove("redundancy_group_id");
			sharedGroupState->Remove("is_reachable");
			sharedGroupState->Remove("last_state_change");

			addDependencyStateToStream(m_PrefixConfigObject + "redundancygroup:state", stateAttrs);
			addDependencyStateToStream(m_PrefixConfigObject + "dependency:edge:state", sharedGroupState);
			AddDataToHmSets(hMSets, RedisKey::RedundancyGroupState, dependencyGroup->GetIcingaDBIdentifier(), stateAttrs);
			AddDataToHmSets(hMSets, RedisKey::DependencyEdgeState, dependencyGroup->GetIcingaDBIdentifier(), sharedGroupState);
		}
	}

	if (!streamStates.empty()) {
		RedisConnection::Queries queries;
		for (auto& [redisKey, query] : hMSets) {
			query.insert(query.begin(), {"HSET", redisKey});
			queries.emplace_back(std::move(query));
		}

		m_Rcon->FireAndForgetQueries(std::move(queries), Prio::RuntimeStateSync);
		m_Rcon->FireAndForgetQueries(std::move(streamStates), Prio::RuntimeStateStream, {0, 1});
	}
}

// Used to update a single object, used for runtime updates
void IcingaDB::SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String typeName = GetLowerCaseTypeNameDB(object);

	std::map<String, std::vector<String>> hMSets;
	std::vector<Dictionary::Ptr> runtimeUpdates;

	CreateConfigUpdate(object, typeName, hMSets, runtimeUpdates, runtimeUpdate);
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (checkable) {
		UpdateState(checkable, runtimeUpdate ? StateUpdate::Full : StateUpdate::Volatile);
	}

	ExecuteRedisTransaction(m_Rcon, hMSets, runtimeUpdates);

	if (checkable) {
		SendNextUpdate(checkable);
	}
}

void IcingaDB::AddObjectDataToRuntimeUpdates(std::vector<Dictionary::Ptr>& runtimeUpdates, const String& objectKey,
		const String& redisKey, const Dictionary::Ptr& data)
{
	Dictionary::Ptr dataClone = data->ShallowClone();
	dataClone->Set("id", objectKey);
	dataClone->Set("redis_key", redisKey);
	dataClone->Set("runtime_type", "upsert");
	runtimeUpdates.emplace_back(dataClone);
}

// Takes object and collects IcingaDB relevant attributes and computes checksums. Returns whether the object is relevant
// for IcingaDB.
bool IcingaDB::PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes, Dictionary::Ptr& checksums)
{
	auto originalAttrs (object->GetOriginalAttributes());

	if (originalAttrs) {
		originalAttrs = originalAttrs->ShallowClone();
	}

	attributes->Set("name_checksum", SHA1(object->GetName()));
	attributes->Set("environment_id", m_EnvironmentId);
	attributes->Set("name", object->GetName());
	attributes->Set("original_attributes", originalAttrs);

	Zone::Ptr ObjectsZone;
	Type::Ptr type = object->GetReflectionType();

	if (type == Endpoint::TypeInstance) {
		ObjectsZone = static_cast<Endpoint*>(object.get())->GetZone();
	} else {
		ObjectsZone = static_pointer_cast<Zone>(object->GetZone());
	}

	if (ObjectsZone) {
		attributes->Set("zone_id", GetObjectIdentifier(ObjectsZone));
		attributes->Set("zone_name", ObjectsZone->GetName());
	}

	if (type == Endpoint::TypeInstance) {
		return true;
	}

	if (type == Zone::TypeInstance) {
		Zone::Ptr zone = static_pointer_cast<Zone>(object);

		attributes->Set("is_global", zone->GetGlobal());

		Zone::Ptr parent = zone->GetParent();
		if (parent) {
			attributes->Set("parent_id", GetObjectIdentifier(parent));
		}

		auto parentsRaw (zone->GetAllParentsRaw());
		attributes->Set("depth", parentsRaw.size());

		return true;
	}

	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);
		auto checkTimeout (checkable->GetCheckTimeout());

		attributes->Set("checkcommand_name", checkable->GetCheckCommand()->GetName());
		attributes->Set("max_check_attempts", checkable->GetMaxCheckAttempts());
		attributes->Set("check_timeout", checkTimeout.IsEmpty() ? checkable->GetCheckCommand()->GetTimeout() : (double)checkTimeout);
		attributes->Set("check_interval", checkable->GetCheckInterval());
		attributes->Set("check_retry_interval", checkable->GetRetryInterval());
		attributes->Set("active_checks_enabled", checkable->GetEnableActiveChecks());
		attributes->Set("passive_checks_enabled", checkable->GetEnablePassiveChecks());
		attributes->Set("event_handler_enabled", checkable->GetEnableEventHandler());
		attributes->Set("notifications_enabled", checkable->GetEnableNotifications());
		attributes->Set("flapping_enabled", checkable->GetEnableFlapping());
		attributes->Set("flapping_threshold_low", checkable->GetFlappingThresholdLow());
		attributes->Set("flapping_threshold_high", checkable->GetFlappingThresholdHigh());
		attributes->Set("perfdata_enabled", checkable->GetEnablePerfdata());
		attributes->Set("is_volatile", checkable->GetVolatile());
		attributes->Set("notes", checkable->GetNotes());
		attributes->Set("icon_image_alt", checkable->GetIconImageAlt());

		if (size_t totalChildren (checkable->GetAllChildrenCount()); totalChildren > 0) {
			// Only set the Redis key if the Checkable has actually some child dependencies.
			attributes->Set("total_children", totalChildren);
		}

		attributes->Set("checkcommand_id", GetObjectIdentifier(checkable->GetCheckCommand()));

		Endpoint::Ptr commandEndpoint = checkable->GetCommandEndpoint();
		if (commandEndpoint) {
			attributes->Set("command_endpoint_id", GetObjectIdentifier(commandEndpoint));
			attributes->Set("command_endpoint_name", commandEndpoint->GetName());
		}

		TimePeriod::Ptr timePeriod = checkable->GetCheckPeriod();
		if (timePeriod) {
			attributes->Set("check_timeperiod_id", GetObjectIdentifier(timePeriod));
			attributes->Set("check_timeperiod_name", timePeriod->GetName());
		}

		EventCommand::Ptr eventCommand = checkable->GetEventCommand();
		if (eventCommand) {
			attributes->Set("eventcommand_id", GetObjectIdentifier(eventCommand));
			attributes->Set("eventcommand_name", eventCommand->GetName());
		}

		String actionUrl = checkable->GetActionUrl();
		String notesUrl = checkable->GetNotesUrl();
		String iconImage = checkable->GetIconImage();
		if (!actionUrl.IsEmpty())
			attributes->Set("action_url_id", HashValue(new Array({m_EnvironmentId, actionUrl})));
		if (!notesUrl.IsEmpty())
			attributes->Set("notes_url_id", HashValue(new Array({m_EnvironmentId, notesUrl})));
		if (!iconImage.IsEmpty())
			attributes->Set("icon_image_id", HashValue(new Array({m_EnvironmentId, iconImage})));


		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		if (service) {
			attributes->Set("host_id", GetObjectIdentifier(service->GetHost()));
			attributes->Set("display_name", service->GetDisplayName());

			// Overwrite name here, `object->name` is 'HostName!ServiceName' but we only want the name of the Service
			attributes->Set("name", service->GetShortName());
		} else {
			attributes->Set("display_name", host->GetDisplayName());
			attributes->Set("address", host->GetAddress());
			attributes->Set("address6", host->GetAddress6());
		}

		return true;
	}

	if (type == User::TypeInstance) {
		User::Ptr user = static_pointer_cast<User>(object);

		attributes->Set("display_name", user->GetDisplayName());
		attributes->Set("email", user->GetEmail());
		attributes->Set("pager", user->GetPager());
		attributes->Set("notifications_enabled", user->GetEnableNotifications());
		attributes->Set("states", user->GetStates());
		attributes->Set("types", user->GetTypes());

		if (user->GetPeriod())
			attributes->Set("timeperiod_id", GetObjectIdentifier(user->GetPeriod()));

		return true;
	}

	if (type == TimePeriod::TypeInstance) {
		TimePeriod::Ptr timeperiod = static_pointer_cast<TimePeriod>(object);

		attributes->Set("display_name", timeperiod->GetDisplayName());
		attributes->Set("prefer_includes", timeperiod->GetPreferIncludes());
		return true;
	}

	if (type == Notification::TypeInstance) {
		Notification::Ptr notification = static_pointer_cast<Notification>(object);

		Host::Ptr host;
		Service::Ptr service;

		tie(host, service) = GetHostService(notification->GetCheckable());

		attributes->Set("notificationcommand_id", GetObjectIdentifier(notification->GetCommand()));

		attributes->Set("host_id", GetObjectIdentifier(host));
		if (service)
			attributes->Set("service_id", GetObjectIdentifier(service));

		TimePeriod::Ptr timeperiod = notification->GetPeriod();
		if (timeperiod)
			attributes->Set("timeperiod_id", GetObjectIdentifier(timeperiod));

		if (notification->GetTimes()) {
			auto begin (notification->GetTimes()->Get("begin"));
			auto end (notification->GetTimes()->Get("end"));

			if (begin != Empty && (double)begin >= 0) {
				attributes->Set("times_begin", std::round((double)begin));
			}

			if (end != Empty && (double)end >= 0) {
				attributes->Set("times_end", std::round((double)end));
			}
		}

		attributes->Set("notification_interval", std::max(0.0, std::round(notification->GetInterval())));
		attributes->Set("states", notification->GetStates());
		attributes->Set("types", notification->GetTypes());

		return true;
	}

	if (type == Comment::TypeInstance) {
		Comment::Ptr comment = static_pointer_cast<Comment>(object);

		attributes->Set("author", comment->GetAuthor());
		attributes->Set("text", comment->GetText());
		attributes->Set("entry_type", comment->GetEntryType());
		attributes->Set("entry_time", TimestampToMilliseconds(comment->GetEntryTime()));
		attributes->Set("is_persistent", comment->GetPersistent());
		attributes->Set("is_sticky", comment->GetSticky());

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(comment->GetCheckable());

		attributes->Set("host_id", GetObjectIdentifier(host));
		if (service) {
			attributes->Set("object_type", "service");
			attributes->Set("service_id", GetObjectIdentifier(service));
		} else
			attributes->Set("object_type", "host");

		auto expireTime (comment->GetExpireTime());

		if (expireTime > 0) {
			attributes->Set("expire_time", TimestampToMilliseconds(expireTime));
		}

		return true;
	}

	if (type == Downtime::TypeInstance) {
		Downtime::Ptr downtime = static_pointer_cast<Downtime>(object);

		attributes->Set("author", downtime->GetAuthor());
		attributes->Set("comment", downtime->GetComment());
		attributes->Set("entry_time", TimestampToMilliseconds(downtime->GetEntryTime()));
		attributes->Set("scheduled_start_time", TimestampToMilliseconds(downtime->GetStartTime()));
		attributes->Set("scheduled_end_time", TimestampToMilliseconds(downtime->GetEndTime()));
		attributes->Set("scheduled_duration", TimestampToMilliseconds(std::max(0.0, downtime->GetEndTime() - downtime->GetStartTime())));
		attributes->Set("flexible_duration", TimestampToMilliseconds(std::max(0.0, downtime->GetDuration())));
		attributes->Set("is_flexible", !downtime->GetFixed());
		attributes->Set("is_in_effect", downtime->IsInEffect());
		if (downtime->IsInEffect()) {
			attributes->Set("start_time", TimestampToMilliseconds(downtime->GetTriggerTime()));

			attributes->Set("end_time", TimestampToMilliseconds(
				downtime->GetFixed() ? downtime->GetEndTime() : (downtime->GetTriggerTime() + std::max(0.0, downtime->GetDuration()))
			));
		}

		auto duration = downtime->GetDuration();
		if (downtime->GetFixed()) {
			duration = downtime->GetEndTime() - downtime->GetStartTime();
		}
		attributes->Set("duration", TimestampToMilliseconds(std::max(0.0, duration)));

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(downtime->GetCheckable());

		attributes->Set("host_id", GetObjectIdentifier(host));
		if (service) {
			attributes->Set("object_type", "service");
			attributes->Set("service_id", GetObjectIdentifier(service));
		} else
			attributes->Set("object_type", "host");

		auto triggeredBy (Downtime::GetByName(downtime->GetTriggeredBy()));
		if (triggeredBy) {
			attributes->Set("triggered_by_id", GetObjectIdentifier(triggeredBy));
		}

		auto scheduledBy (downtime->GetScheduledBy());
		if (!scheduledBy.IsEmpty()) {
			attributes->Set("scheduled_by", scheduledBy);
		}

		auto parent (Downtime::GetByName(downtime->GetParent()));
		if (parent) {
			attributes->Set("parent_id", GetObjectIdentifier(parent));
		}

		return true;
	}

	if (type == UserGroup::TypeInstance) {
		UserGroup::Ptr userGroup = static_pointer_cast<UserGroup>(object);

		attributes->Set("display_name", userGroup->GetDisplayName());

		return true;
	}

	if (type == HostGroup::TypeInstance) {
		HostGroup::Ptr hostGroup = static_pointer_cast<HostGroup>(object);

		attributes->Set("display_name", hostGroup->GetDisplayName());

		return true;
	}

	if (type == ServiceGroup::TypeInstance) {
		ServiceGroup::Ptr serviceGroup = static_pointer_cast<ServiceGroup>(object);

		attributes->Set("display_name", serviceGroup->GetDisplayName());

		return true;
	}

	if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance) {
		Command::Ptr command = static_pointer_cast<Command>(object);

		attributes->Set("command", JsonEncode(command->GetCommandLine()));
		attributes->Set("timeout", std::max(0, command->GetTimeout()));

		return true;
	}

	return false;
}

/* Creates a config update with computed checksums etc.
 * Writes attributes, customVars and checksums into the respective supplied vectors. Adds two values to each vector
 * (if applicable), first the key then the value. To use in a Redis command the command (e.g. HSET) and the key (e.g.
 * icinga:config:object:downtime) need to be prepended. There is nothing to indicate success or failure.
 */
void
IcingaDB::CreateConfigUpdate(const ConfigObject::Ptr& object, const String typeName, std::map<String, std::vector<String>>& hMSets,
								std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate)
{
	/* TODO: This isn't essentially correct as we don't keep track of config objects ourselves. This would avoid duplicated config updates at startup.
	if (!runtimeUpdate && m_ConfigDumpInProgress)
		return;
	*/

	if (m_Rcon == nullptr)
		return;

	Dictionary::Ptr attr = new Dictionary;
	Dictionary::Ptr chksm = new Dictionary;

	if (!PrepareObject(object, attr, chksm))
		return;

	InsertObjectDependencies(object, typeName, hMSets, runtimeUpdates, runtimeUpdate);

	String objectKey = GetObjectIdentifier(object);
	auto& attrs (hMSets[m_PrefixConfigObject + typeName]);
	auto& chksms (hMSets[m_PrefixConfigCheckSum + typeName]);

	attrs.emplace_back(objectKey);
	attrs.emplace_back(JsonEncode(attr));

	String checksum = HashValue(attr);
	chksms.emplace_back(objectKey);
	chksms.emplace_back(JsonEncode(new Dictionary({{"checksum", checksum}})));

	/* Send an update event to subscribers. */
	if (runtimeUpdate) {
		attr->Set("checksum", checksum);
		AddObjectDataToRuntimeUpdates(runtimeUpdates, objectKey, m_PrefixConfigObject + typeName, attr);
	}
}

void IcingaDB::SendConfigDelete(const ConfigObject::Ptr& object)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Type::Ptr type = object->GetReflectionType();
	String typeName = type->GetName().ToLower();
	String objectKey = GetObjectIdentifier(object);

	m_Rcon->FireAndForgetQueries({
		{"HDEL", m_PrefixConfigObject + typeName, objectKey},
		{"HDEL", m_PrefixConfigCheckSum + typeName, objectKey},
		{
			"XADD", "icinga:runtime", "MAXLEN", "~", "1000000", "*",
			"redis_key", m_PrefixConfigObject + typeName, "id", objectKey, "runtime_type", "delete"
		}
   	}, Prio::Config);

	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);

	if (customVarObject) {
		Dictionary::Ptr vars = customVarObject->GetVars();
		SendCustomVarsChanged(object, vars, nullptr);
	}

	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		m_Rcon->FireAndForgetQuery({
			"ZREM",
			service ? "icinga:nextupdate:service" : "icinga:nextupdate:host",
			GetObjectIdentifier(checkable)
		}, Prio::CheckResult);

		m_Rcon->FireAndForgetQueries({
			{"HDEL", m_PrefixConfigObject + typeName + ":state", objectKey},
			{"HDEL", m_PrefixConfigCheckSum + typeName + ":state", objectKey}
		}, Prio::RuntimeStateSync);

		if (service) {
			SendGroupsChanged<ServiceGroup>(checkable, service->GetGroups(), nullptr);
		} else {
			SendGroupsChanged<HostGroup>(checkable, host->GetGroups(), nullptr);
		}

		return;
	}

	if (type == TimePeriod::TypeInstance) {
		TimePeriod::Ptr timeperiod = static_pointer_cast<TimePeriod>(object);
		SendTimePeriodRangesChanged(timeperiod, timeperiod->GetRanges(), nullptr);
		SendTimePeriodIncludesChanged(timeperiod, timeperiod->GetIncludes(), nullptr);
		SendTimePeriodExcludesChanged(timeperiod, timeperiod->GetExcludes(), nullptr);
		return;
	}

	if (type == User::TypeInstance) {
		User::Ptr user = static_pointer_cast<User>(object);
		SendGroupsChanged<UserGroup>(user, user->GetGroups(), nullptr);
		return;
	}

	if (type == Notification::TypeInstance) {
		Notification::Ptr notification = static_pointer_cast<Notification>(object);
		SendNotificationUsersChanged(notification, notification->GetUsersRaw(), nullptr);
		SendNotificationUserGroupsChanged(notification, notification->GetUserGroupsRaw(), nullptr);
		return;
	}

	if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance) {
		Command::Ptr command = static_pointer_cast<Command>(object);
		SendCommandArgumentsChanged(command, command->GetArguments(), nullptr);
		SendCommandEnvChanged(command, command->GetEnv(), nullptr);
		return;
	}
}

static inline
unsigned short GetPreviousState(const Checkable::Ptr& checkable, const Service::Ptr& service, StateType type)
{
	auto phs ((type == StateTypeHard ? checkable->GetLastHardStatesRaw() : checkable->GetLastSoftStatesRaw()) % 100u);

	if (service) {
		return phs;
	} else {
		return phs == 99 ? phs : Host::CalculateState(ServiceState(phs));
	}
}

void IcingaDB::SendStateChange(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type)
{
	if (!GetActive()) {
		return;
	}

	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (!checkable)
		return;

	if (!cr)
		return;

	Host::Ptr host;
	Service::Ptr service;

	tie(host, service) = GetHostService(checkable);

	UpdateState(checkable, StateUpdate::RuntimeOnly);

	int hard_state;
	if (!cr) {
		hard_state = 99;
	} else {
		hard_state = service ? Convert::ToLong(service->GetLastHardState()) : Convert::ToLong(host->GetLastHardState());
	}

	auto eventTime (cr->GetExecutionEnd());
	auto eventTs (TimestampToMilliseconds(eventTime));

	Array::Ptr rawId = new Array({m_EnvironmentId, object->GetName()});
	rawId->Add(eventTs);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:state", "*",
		"id", HashValue(rawId),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"state_type", Convert::ToString(type),
		"soft_state", Convert::ToString(cr ? service ? Convert::ToLong(cr->GetState()) : Convert::ToLong(Host::CalculateState(cr->GetState())) : 99),
		"hard_state", Convert::ToString(hard_state),
		"check_attempt", Convert::ToString(checkable->GetCheckAttempt()),
		"previous_soft_state", Convert::ToString(GetPreviousState(checkable, service, StateTypeSoft)),
		"previous_hard_state", Convert::ToString(GetPreviousState(checkable, service, StateTypeHard)),
		"max_check_attempts", Convert::ToString(checkable->GetMaxCheckAttempts()),
		"event_time", Convert::ToString(eventTs),
		"event_id", CalcEventID("state_change", object, eventTime),
		"event_type", "state_change"
	});

	if (cr) {
		auto output (cr->GetOutput());
		auto pos (output.Find("\n"));

		if (pos != String::NPos) {
			auto longOutput (output.SubStr(pos + 1u));
			output.erase(output.Begin() + pos, output.End());

			xAdd.emplace_back("long_output");
			xAdd.emplace_back(Utility::ValidateUTF8(std::move(longOutput)));
		}

		xAdd.emplace_back("output");
		xAdd.emplace_back(Utility::ValidateUTF8(std::move(output)));
		xAdd.emplace_back("check_source");
		xAdd.emplace_back(cr->GetCheckSource());
		xAdd.emplace_back("scheduling_source");
		xAdd.emplace_back(cr->GetSchedulingSource());
	}

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::SendSentNotification(
	const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
	NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text, double sendTime
)
{
	if (!GetActive()) {
		return;
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	auto finalText = text;
	if (finalText == "" && cr) {
		finalText = cr->GetOutput();
	}

	auto usersAmount (users.size());
	auto sendTs (TimestampToMilliseconds(sendTime));

	Array::Ptr rawId = new Array({m_EnvironmentId, notification->GetName()});
	rawId->Add(GetNotificationTypeByEnum(type));
	rawId->Add(sendTs);

	auto notificationHistoryId (HashValue(rawId));

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:notification", "*",
		"id", notificationHistoryId,
		"environment_id", m_EnvironmentId,
		"notification_id", GetObjectIdentifier(notification),
		"host_id", GetObjectIdentifier(host),
		"type", Convert::ToString(type),
		"state", Convert::ToString(cr ? service ? Convert::ToLong(cr->GetState()) : Convert::ToLong(Host::CalculateState(cr->GetState())) : 99),
		"previous_hard_state", Convert::ToString(cr ? Convert::ToLong(service ? cr->GetPreviousHardState() : Host::CalculateState(cr->GetPreviousHardState())) : 99),
		"author", Utility::ValidateUTF8(author),
		"text", Utility::ValidateUTF8(finalText),
		"users_notified", Convert::ToString(usersAmount),
		"send_time", Convert::ToString(sendTs),
		"event_id", CalcEventID("notification", notification, sendTime, type),
		"event_type", "notification"
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	if (!users.empty()) {
		Array::Ptr users_notified = new Array();
		for (const User::Ptr& user : users) {
			users_notified->Add(GetObjectIdentifier(user));
		}
		xAdd.emplace_back("users_notified_ids");
		xAdd.emplace_back(JsonEncode(users_notified));
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::SendStartedDowntime(const Downtime::Ptr& downtime)
{
	if (!GetActive()) {
		return;
	}

	SendConfigUpdate(downtime, true);

	auto checkable (downtime->GetCheckable());
	auto triggeredBy (Downtime::GetByName(downtime->GetTriggeredBy()));

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	/* Update checkable state as in_downtime may have changed. */
	UpdateState(checkable, StateUpdate::Full);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:downtime", "*",
		"downtime_id", GetObjectIdentifier(downtime),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"entry_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEntryTime())),
		"author", Utility::ValidateUTF8(downtime->GetAuthor()),
		"comment", Utility::ValidateUTF8(downtime->GetComment()),
		"is_flexible", Convert::ToString((unsigned short)!downtime->GetFixed()),
		"flexible_duration", Convert::ToString(TimestampToMilliseconds(std::max(0.0, downtime->GetDuration()))),
		"scheduled_start_time", Convert::ToString(TimestampToMilliseconds(downtime->GetStartTime())),
		"scheduled_end_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEndTime())),
		"has_been_cancelled", Convert::ToString((unsigned short)downtime->GetWasCancelled()),
		"trigger_time", Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime())),
		"cancel_time", Convert::ToString(TimestampToMilliseconds(downtime->GetRemoveTime())),
		"event_id", CalcEventID("downtime_start", downtime),
		"event_type", "downtime_start"
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	if (triggeredBy) {
		xAdd.emplace_back("triggered_by_id");
		xAdd.emplace_back(GetObjectIdentifier(triggeredBy));
	}

	if (downtime->GetFixed()) {
		xAdd.emplace_back("start_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetStartTime())));
		xAdd.emplace_back("end_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetEndTime())));
	} else {
		xAdd.emplace_back("start_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime())));
		xAdd.emplace_back("end_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime() + std::max(0.0, downtime->GetDuration()))));
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	auto parent (Downtime::GetByName(downtime->GetParent()));

	if (parent) {
		xAdd.emplace_back("parent_id");
		xAdd.emplace_back(GetObjectIdentifier(parent));
	}

	auto scheduledBy (downtime->GetScheduledBy());

	if (!scheduledBy.IsEmpty()) {
		xAdd.emplace_back("scheduled_by");
		xAdd.emplace_back(scheduledBy);
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::SendRemovedDowntime(const Downtime::Ptr& downtime)
{
	if (!GetActive()) {
		return;
	}

	auto checkable (downtime->GetCheckable());
	auto triggeredBy (Downtime::GetByName(downtime->GetTriggeredBy()));

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	// Downtime never got triggered (didn't send "downtime_start") so we don't want to send "downtime_end"
	if (downtime->GetTriggerTime() == 0)
		return;

	/* Update checkable state as in_downtime may have changed. */
	UpdateState(checkable, StateUpdate::Full);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:downtime", "*",
		"downtime_id", GetObjectIdentifier(downtime),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"entry_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEntryTime())),
		"author", Utility::ValidateUTF8(downtime->GetAuthor()),
		"cancelled_by", Utility::ValidateUTF8(downtime->GetRemovedBy()),
		"comment", Utility::ValidateUTF8(downtime->GetComment()),
		"is_flexible", Convert::ToString((unsigned short)!downtime->GetFixed()),
		"flexible_duration", Convert::ToString(TimestampToMilliseconds(std::max(0.0, downtime->GetDuration()))),
		"scheduled_start_time", Convert::ToString(TimestampToMilliseconds(downtime->GetStartTime())),
		"scheduled_end_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEndTime())),
		"has_been_cancelled", Convert::ToString((unsigned short)downtime->GetWasCancelled()),
		"trigger_time", Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime())),
		"cancel_time", Convert::ToString(TimestampToMilliseconds(downtime->GetRemoveTime())),
		"event_id", CalcEventID("downtime_end", downtime),
		"event_type", "downtime_end"
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	if (triggeredBy) {
		xAdd.emplace_back("triggered_by_id");
		xAdd.emplace_back(GetObjectIdentifier(triggeredBy));
	}

	if (downtime->GetFixed()) {
		xAdd.emplace_back("start_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetStartTime())));
		xAdd.emplace_back("end_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetEndTime())));
	} else {
		xAdd.emplace_back("start_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime())));
		xAdd.emplace_back("end_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime() + std::max(0.0, downtime->GetDuration()))));
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	auto parent (Downtime::GetByName(downtime->GetParent()));

	if (parent) {
		xAdd.emplace_back("parent_id");
		xAdd.emplace_back(GetObjectIdentifier(parent));
	}

	auto scheduledBy (downtime->GetScheduledBy());

	if (!scheduledBy.IsEmpty()) {
		xAdd.emplace_back("scheduled_by");
		xAdd.emplace_back(scheduledBy);
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::SendAddedComment(const Comment::Ptr& comment)
{
	if (comment->GetEntryType() != CommentUser || !GetActive())
		return;

	auto checkable (comment->GetCheckable());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:comment", "*",
		"comment_id", GetObjectIdentifier(comment),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"entry_time", Convert::ToString(TimestampToMilliseconds(comment->GetEntryTime())),
		"author", Utility::ValidateUTF8(comment->GetAuthor()),
		"comment", Utility::ValidateUTF8(comment->GetText()),
		"entry_type", Convert::ToString(comment->GetEntryType()),
		"is_persistent", Convert::ToString((unsigned short)comment->GetPersistent()),
		"is_sticky", Convert::ToString((unsigned short)comment->GetSticky()),
		"event_id", CalcEventID("comment_add", comment),
		"event_type", "comment_add"
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	{
		auto expireTime (comment->GetExpireTime());

		if (expireTime > 0) {
			xAdd.emplace_back("expire_time");
			xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(expireTime)));
		}
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
	UpdateState(checkable, StateUpdate::Full);
}

void IcingaDB::SendRemovedComment(const Comment::Ptr& comment)
{
	if (comment->GetEntryType() != CommentUser || !GetActive()) {
		return;
	}

	double removeTime = comment->GetRemoveTime();
	bool wasRemoved = removeTime > 0;

	double expireTime = comment->GetExpireTime();
	bool hasExpireTime = expireTime > 0;
	bool isExpired = hasExpireTime && expireTime <= Utility::GetTime();

	if (!wasRemoved && !isExpired) {
		/* The comment object disappeared for no apparent reason, most likely because it simply was deleted instead
		 * of using the proper remove-comment API action. In this case, information that should normally be set is
		 * missing and a proper history event cannot be generated.
		 */
		return;
	}

	auto checkable (comment->GetCheckable());

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:comment", "*",
		"comment_id", GetObjectIdentifier(comment),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"entry_time", Convert::ToString(TimestampToMilliseconds(comment->GetEntryTime())),
		"author", Utility::ValidateUTF8(comment->GetAuthor()),
		"comment", Utility::ValidateUTF8(comment->GetText()),
		"entry_type", Convert::ToString(comment->GetEntryType()),
		"is_persistent", Convert::ToString((unsigned short)comment->GetPersistent()),
		"is_sticky", Convert::ToString((unsigned short)comment->GetSticky()),
		"event_id", CalcEventID("comment_remove", comment),
		"event_type", "comment_remove"
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	if (wasRemoved) {
		xAdd.emplace_back("remove_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(removeTime)));
		xAdd.emplace_back("has_been_removed");
		xAdd.emplace_back("1");
		xAdd.emplace_back("removed_by");
		xAdd.emplace_back(Utility::ValidateUTF8(comment->GetRemovedBy()));
	} else {
		xAdd.emplace_back("has_been_removed");
		xAdd.emplace_back("0");
	}

	if (hasExpireTime) {
		xAdd.emplace_back("expire_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(expireTime)));
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
	UpdateState(checkable, StateUpdate::Full);
}

void IcingaDB::SendFlappingChange(const Checkable::Ptr& checkable, double changeTime, double flappingLastChange)
{
	if (!GetActive()) {
		return;
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:flapping", "*",
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"flapping_threshold_low", Convert::ToString(checkable->GetFlappingThresholdLow()),
		"flapping_threshold_high", Convert::ToString(checkable->GetFlappingThresholdHigh())
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	long long startTime;

	if (checkable->IsFlapping()) {
		startTime = TimestampToMilliseconds(changeTime);

		xAdd.emplace_back("event_type");
		xAdd.emplace_back("flapping_start");
		xAdd.emplace_back("percent_state_change_start");
		xAdd.emplace_back(Convert::ToString(checkable->GetFlappingCurrent()));
	} else {
		startTime = TimestampToMilliseconds(flappingLastChange);

		xAdd.emplace_back("event_type");
		xAdd.emplace_back("flapping_end");
		xAdd.emplace_back("end_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(changeTime)));
		xAdd.emplace_back("percent_state_change_end");
		xAdd.emplace_back(Convert::ToString(checkable->GetFlappingCurrent()));
	}

	xAdd.emplace_back("start_time");
	xAdd.emplace_back(Convert::ToString(startTime));
	xAdd.emplace_back("event_id");
	xAdd.emplace_back(CalcEventID(checkable->IsFlapping() ? "flapping_start" : "flapping_end", checkable, startTime));
	xAdd.emplace_back("id");
	xAdd.emplace_back(HashValue(new Array({m_EnvironmentId, checkable->GetName(), startTime})));

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::SendNextUpdate(const Checkable::Ptr& checkable)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	if (checkable->GetEnableActiveChecks()) {
		m_Rcon->FireAndForgetQuery(
			{
				"ZADD",
				dynamic_pointer_cast<Service>(checkable) ? "icinga:nextupdate:service" : "icinga:nextupdate:host",
				Convert::ToString(checkable->GetNextUpdate()),
				GetObjectIdentifier(checkable)
			},
			Prio::CheckResult
		);
	} else {
		m_Rcon->FireAndForgetQuery(
			{
				"ZREM",
				dynamic_pointer_cast<Service>(checkable) ? "icinga:nextupdate:service" : "icinga:nextupdate:host",
				GetObjectIdentifier(checkable)
			},
			Prio::CheckResult
		);
	}
}

void IcingaDB::SendAcknowledgementSet(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool persistent, double changeTime, double expiry)
{
	if (!GetActive()) {
		return;
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	/* Update checkable state as is_acknowledged may have changed. */
	UpdateState(checkable, StateUpdate::Full);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:acknowledgement", "*",
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"event_type", "ack_set",
		"author", author,
		"comment", comment,
		"is_sticky", Convert::ToString((unsigned short)(type == AcknowledgementSticky)),
		"is_persistent", Convert::ToString((unsigned short)persistent)
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	if (expiry > 0) {
		xAdd.emplace_back("expire_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(expiry)));
	}

	long long setTime = TimestampToMilliseconds(changeTime);

	xAdd.emplace_back("set_time");
	xAdd.emplace_back(Convert::ToString(setTime));
	xAdd.emplace_back("event_id");
	xAdd.emplace_back(CalcEventID("ack_set", checkable, setTime));
	xAdd.emplace_back("id");
	xAdd.emplace_back(HashValue(new Array({m_EnvironmentId, checkable->GetName(), setTime})));

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::SendAcknowledgementCleared(const Checkable::Ptr& checkable, const String& removedBy, double changeTime, double ackLastChange)
{
	if (!GetActive()) {
		return;
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	/* Update checkable state as is_acknowledged may have changed. */
	UpdateState(checkable, StateUpdate::Full);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:acknowledgement", "*",
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"clear_time", Convert::ToString(TimestampToMilliseconds(changeTime)),
		"event_type", "ack_clear"
	});

	if (service) {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("service");
		xAdd.emplace_back("service_id");
		xAdd.emplace_back(GetObjectIdentifier(checkable));
	} else {
		xAdd.emplace_back("object_type");
		xAdd.emplace_back("host");
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	long long setTime = TimestampToMilliseconds(ackLastChange);

	xAdd.emplace_back("set_time");
	xAdd.emplace_back(Convert::ToString(setTime));
	xAdd.emplace_back("event_id");
	xAdd.emplace_back(CalcEventID("ack_clear", checkable, setTime));
	xAdd.emplace_back("id");
	xAdd.emplace_back(HashValue(new Array({m_EnvironmentId, checkable->GetName(), setTime})));

	if (!removedBy.IsEmpty()) {
		xAdd.emplace_back("cleared_by");
		xAdd.emplace_back(removedBy);
	}

	m_HistoryBulker.ProduceOne(std::move(xAdd));
}

void IcingaDB::ForwardHistoryEntries()
{
	using clock = std::chrono::steady_clock;

	const std::chrono::seconds logInterval (10);
	auto nextLog (clock::now() + logInterval);

	auto logPeriodically ([this, logInterval, &nextLog]() {
		if (clock::now() > nextLog) {
			nextLog += logInterval;

			auto size (m_HistoryBulker.Size());

			Log(size > m_HistoryBulker.GetBulkSize() ? LogInformation : LogNotice, "IcingaDB")
				<< "Pending history queries: " << size;
		}
	});

	for (;;) {
		logPeriodically();

		auto haystack (m_HistoryBulker.ConsumeMany());

		if (haystack.empty()) {
			if (!GetActive()) {
				break;
			}

			continue;
		}

		uintmax_t attempts = 0;

		auto logFailure ([&haystack, &attempts](const char* err = nullptr) {
			Log msg (LogNotice, "IcingaDB");

			msg << "history: " << haystack.size() << " queries failed temporarily (attempt #" << ++attempts << ")";

			if (err) {
				msg << ": " << err;
			}
		});

		for (;;) {
			logPeriodically();

			if (m_Rcon && m_Rcon->IsConnected()) {
				try {
					m_Rcon->GetResultsOfQueries(haystack, Prio::History, {0, 0, haystack.size()});
					break;
				} catch (const std::exception& ex) {
					logFailure(ex.what());
				} catch (...) {
					logFailure();
				}
			} else {
				logFailure("not connected to Redis");
			}

			if (!GetActive()) {
				Log(LogCritical, "IcingaDB") << "history: " << haystack.size() << " queries failed (attempt #" << attempts
					<< ") while we're about to shut down. Giving up and discarding additional "
					<< m_HistoryBulker.Size() << " queued history queries.";

				return;
			}

			Utility::Sleep(2);
		}
	}
}

void IcingaDB::SendNotificationUsersChanged(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<Value> deletedUsers = GetArrayDeletedValues(oldValues, newValues);

	for (const auto& userName : deletedUsers) {
		String id = HashValue(new Array({m_EnvironmentId, "user", userName, notification->GetName()}));
		DeleteRelationship(id, "notification:user");
		DeleteRelationship(id, "notification:recipient");
	}
}

void IcingaDB::SendNotificationUserGroupsChanged(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<Value> deletedUserGroups = GetArrayDeletedValues(oldValues, newValues);

	for (const auto& userGroupName : deletedUserGroups) {
		UserGroup::Ptr userGroup = UserGroup::GetByName(userGroupName);
		String id = HashValue(new Array({m_EnvironmentId, "usergroup", userGroupName, notification->GetName()}));
		DeleteRelationship(id, "notification:usergroup");
		DeleteRelationship(id, "notification:recipient");

		for (const User::Ptr& user : userGroup->GetMembers()) {
			String userId = HashValue(new Array({m_EnvironmentId, "usergroupuser", user->GetName(), userGroupName, notification->GetName()}));
			DeleteRelationship(userId, "notification:recipient");
		}
	}
}

void IcingaDB::SendTimePeriodRangesChanged(const TimePeriod::Ptr& timeperiod, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<String> deletedKeys = GetDictionaryDeletedKeys(oldValues, newValues);
	String typeName = GetLowerCaseTypeNameDB(timeperiod);

	for (const auto& rangeKey : deletedKeys) {
		String id = HashValue(new Array({m_EnvironmentId, rangeKey, oldValues->Get(rangeKey), timeperiod->GetName()}));
		DeleteRelationship(id, "timeperiod:range");
	}
}

void IcingaDB::SendTimePeriodIncludesChanged(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<Value> deletedIncludes = GetArrayDeletedValues(oldValues, newValues);

	for (const auto& includeName : deletedIncludes) {
		String id = HashValue(new Array({m_EnvironmentId, includeName, timeperiod->GetName()}));
		DeleteRelationship(id, "timeperiod:override:include");
	}
}

void IcingaDB::SendTimePeriodExcludesChanged(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<Value> deletedExcludes = GetArrayDeletedValues(oldValues, newValues);

	for (const auto& excludeName : deletedExcludes) {
		String id = HashValue(new Array({m_EnvironmentId, excludeName, timeperiod->GetName()}));
		DeleteRelationship(id, "timeperiod:override:exclude");
	}
}

template<typename T>
void IcingaDB::SendGroupsChanged(const ConfigObject::Ptr& object, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<Value> deletedGroups = GetArrayDeletedValues(oldValues, newValues);
	String typeName = GetLowerCaseTypeNameDB(object);

	for (const auto& groupName : deletedGroups) {
		typename T::Ptr group = ConfigObject::GetObject<T>(groupName);
		String id = HashValue(new Array({m_EnvironmentId, group->GetName(), object->GetName()}));
		DeleteRelationship(id, typeName + "group:member");

		if (std::is_same<T, UserGroup>::value) {
			UserGroup::Ptr userGroup = dynamic_pointer_cast<UserGroup>(group);

			for (const auto& notification : userGroup->GetNotifications()) {
				String userId = HashValue(new Array({m_EnvironmentId, "usergroupuser", object->GetName(), groupName, notification->GetName()}));
				DeleteRelationship(userId, "notification:recipient");
			}
		}
	}
}

void IcingaDB::SendCommandEnvChanged(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<String> deletedKeys = GetDictionaryDeletedKeys(oldValues, newValues);
	String typeName = GetLowerCaseTypeNameDB(command);

	for (const auto& envvarKey : deletedKeys) {
		String id = HashValue(new Array({m_EnvironmentId, envvarKey, command->GetName()}));
		DeleteRelationship(id, typeName + ":envvar", true);
	}
}

void IcingaDB::SendCommandArgumentsChanged(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	std::vector<String> deletedKeys = GetDictionaryDeletedKeys(oldValues, newValues);
	String typeName = GetLowerCaseTypeNameDB(command);

	for (const auto& argumentKey : deletedKeys) {
		String id = HashValue(new Array({m_EnvironmentId, argumentKey, command->GetName()}));
		DeleteRelationship(id, typeName + ":argument", true);
	}
}

void IcingaDB::SendCustomVarsChanged(const ConfigObject::Ptr& object, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	if (m_IndexedTypes.find(object->GetReflectionType().get()) == m_IndexedTypes.end()) {
		return;
	}

	if (!m_Rcon || !m_Rcon->IsConnected() || oldValues == newValues) {
		return;
	}

	Dictionary::Ptr oldVars = SerializeVars(oldValues);
	Dictionary::Ptr newVars = SerializeVars(newValues);

	std::vector<String> deletedVars = GetDictionaryDeletedKeys(oldVars, newVars);
	String typeName = GetLowerCaseTypeNameDB(object);

	for (const auto& varId : deletedVars) {
		String id = HashValue(new Array({m_EnvironmentId, varId, object->GetName()}));
		DeleteRelationship(id, typeName + ":customvar");
	}
}

void IcingaDB::SendDependencyGroupChildRegistered(const Checkable::Ptr& child, const DependencyGroup::Ptr& dependencyGroup)
{
	if (!m_Rcon || !m_Rcon->IsConnected()) {
		return;
	}

	std::vector<Dictionary::Ptr> runtimeUpdates;
	std::map<String, RedisConnection::Query> hMSets;
	InsertCheckableDependencies(child, hMSets, &runtimeUpdates, dependencyGroup);
	ExecuteRedisTransaction(m_Rcon, hMSets, runtimeUpdates);

	UpdateState(child, StateUpdate::Full);
	UpdateDependenciesState(child, dependencyGroup);

	std::set<Checkable::Ptr> parents;
	dependencyGroup->LoadParents(parents);
	for (const auto& parent : parents) {
		// The total_children and affects_children columns might now have different outcome, so update the parent
		// Checkable as well. The grandparent Checkable may still have wrong numbers of total children, though it's not
		// worth traversing the whole tree way up and sending config updates for each one of them, as the next Redis
		// config dump is going to fix it anyway.
		SendConfigUpdate(parent, true);
	}
}

void IcingaDB::SendDependencyGroupChildRemoved(
	const DependencyGroup::Ptr& dependencyGroup,
	const std::vector<Dependency::Ptr>& dependencies,
	bool removeGroup
)
{
	if (!m_Rcon || !m_Rcon->IsConnected() || dependencies.empty()) {
		return;
	}

	Checkable::Ptr child;
	std::set<Checkable*> detachedParents;
	for (const auto& dependency : dependencies) {
		child = dependency->GetChild(); // All dependencies have the same child.
		const auto& parent(dependency->GetParent());
		if (auto [_, inserted] = detachedParents.insert(dependency->GetParent().get()); inserted) {
			String edgeId;
			if (dependencyGroup->IsRedundancyGroup()) {
				// If the redundancy group has no members left, it's going to be removed as well, so we need to
				// delete dependency edges from that group to the parent Checkables.
				if (removeGroup) {
					auto id(HashValue(new Array{dependencyGroup->GetIcingaDBIdentifier(), GetObjectIdentifier(parent)}));
					DeleteRelationship(id, RedisKey::DependencyEdge);
					DeleteState(id, RedisKey::DependencyEdgeState);
				}

				// Remove the connection from the child Checkable to the redundancy group.
				edgeId = HashValue(new Array{GetObjectIdentifier(child), dependencyGroup->GetIcingaDBIdentifier()});
			} else {
				// Remove the edge between the parent and child Checkable linked through the removed dependency.
				edgeId = HashValue(new Array{GetObjectIdentifier(child), GetObjectIdentifier(parent)});
			}

			DeleteRelationship(edgeId, RedisKey::DependencyEdge);

			// The total_children and affects_children columns might now have different outcome, so update the parent
			// Checkable as well. The grandparent Checkable may still have wrong numbers of total children, though it's
			// not worth traversing the whole tree way up and sending config updates for each one of them, as the next
			// Redis config dump is going to fix it anyway.
			SendConfigUpdate(parent, true);

			if (!parent->HasAnyDependencies()) {
				// If the parent Checkable isn't part of any other dependency chain anymore, drop its dependency node entry.
				DeleteRelationship(GetObjectIdentifier(parent), RedisKey::DependencyNode);
			}
		}
	}

	if (removeGroup && dependencyGroup->IsRedundancyGroup()) {
		String redundancyGroupId(dependencyGroup->GetIcingaDBIdentifier());
		DeleteRelationship(redundancyGroupId, RedisKey::DependencyNode);
		DeleteRelationship(redundancyGroupId, RedisKey::RedundancyGroup);

		DeleteState(redundancyGroupId, RedisKey::RedundancyGroupState);
		DeleteState(redundancyGroupId, RedisKey::DependencyEdgeState);
	} else if (removeGroup) {
		// Note: The Icinga DB identifier of a non-redundant dependency group is used as the edge state ID
		// and shared by all of its dependency objects. See also SerializeDependencyEdgeState() for details.
		DeleteState(dependencyGroup->GetIcingaDBIdentifier(), RedisKey::DependencyEdgeState);
	}

	if (!child->HasAnyDependencies()) {
		// If the child Checkable has no parent and reverse dependencies, we can safely remove the dependency node.
		DeleteRelationship(GetObjectIdentifier(child), RedisKey::DependencyNode);
	}
}

Dictionary::Ptr IcingaDB::SerializeState(const Checkable::Ptr& checkable)
{
	Dictionary::Ptr attrs = new Dictionary();

	Host::Ptr host;
	Service::Ptr service;

	tie(host, service) = GetHostService(checkable);

	String id = GetObjectIdentifier(checkable);

	/*
	 * As there is a 1:1 relationship between host and host state, the host ID ('host_id')
	 * is also used as the host state ID ('id'). These are duplicated to 1) avoid having
	 * special handling for this in Icinga DB and 2) to have both a primary key and a foreign key
	 * in the SQL database in the end. In the database 'host_id' ends up as foreign key 'host_state.host_id'
	 * referring to 'host.id' while 'id' ends up as the primary key 'host_state.id'. This also applies for service.
	 */
	attrs->Set("id", id);
	attrs->Set("environment_id", m_EnvironmentId);
	attrs->Set("state_type", checkable->HasBeenChecked() ? checkable->GetStateType() : StateTypeHard);

	// TODO: last_hard/soft_state should be "previous".
	if (service) {
		attrs->Set("service_id", id);
		auto state = service->HasBeenChecked() ? service->GetState() : 99;
		attrs->Set("soft_state", state);
		attrs->Set("hard_state", service->HasBeenChecked() ? service->GetLastHardState() : 99);
		attrs->Set("severity", service->GetSeverity());
		attrs->Set("host_id", GetObjectIdentifier(host));
	} else {
		attrs->Set("host_id", id);
		auto state = host->HasBeenChecked() ? host->GetState() : 99;
		attrs->Set("soft_state", state);
		attrs->Set("hard_state", host->HasBeenChecked() ? host->GetLastHardState() : 99);
		attrs->Set("severity", host->GetSeverity());
	}

	attrs->Set("previous_soft_state", GetPreviousState(checkable, service, StateTypeSoft));
	attrs->Set("previous_hard_state", GetPreviousState(checkable, service, StateTypeHard));
	attrs->Set("check_attempt", checkable->GetCheckAttempt());

	attrs->Set("is_active", checkable->IsActive());
	attrs->Set("affects_children", checkable->AffectsChildren());

	CheckResult::Ptr cr = checkable->GetLastCheckResult();

	if (cr) {
		String rawOutput = cr->GetOutput();
		if (!rawOutput.IsEmpty()) {
			size_t lineBreak = rawOutput.Find("\n");
			String output = rawOutput.SubStr(0, lineBreak);
			if (!output.IsEmpty())
				attrs->Set("output", rawOutput.SubStr(0, lineBreak));

			if (lineBreak > 0 && lineBreak != String::NPos) {
				String longOutput = rawOutput.SubStr(lineBreak+1, rawOutput.GetLength());
				if (!longOutput.IsEmpty())
					attrs->Set("long_output", longOutput);
			}
		}

		String perfData = PluginUtility::FormatPerfdata(cr->GetPerformanceData());
		if (!perfData.IsEmpty())
			attrs->Set("performance_data", perfData);

		String normedPerfData = PluginUtility::FormatPerfdata(cr->GetPerformanceData(), true);
		if (!normedPerfData.IsEmpty())
			attrs->Set("normalized_performance_data", normedPerfData);

		if (!cr->GetCommand().IsEmpty())
			attrs->Set("check_commandline", FormatCommandLine(cr->GetCommand()));
		attrs->Set("execution_time", std::min((long long)UINT32_MAX, TimestampToMilliseconds(fmax(0.0, cr->CalculateExecutionTime()))));
		attrs->Set("latency", std::min((long long)UINT32_MAX, TimestampToMilliseconds(cr->CalculateLatency())));
		attrs->Set("check_source", cr->GetCheckSource());
		attrs->Set("scheduling_source", cr->GetSchedulingSource());
	}

	attrs->Set("is_problem", checkable->GetProblem());
	attrs->Set("is_handled", checkable->GetHandled());
	attrs->Set("is_reachable", checkable->IsReachable());
	attrs->Set("is_flapping", checkable->IsFlapping());

	attrs->Set("is_acknowledged", checkable->GetAcknowledgement());
	if (checkable->IsAcknowledged()) {
		Timestamp entry = 0;
		Comment::Ptr AckComment;
		for (const Comment::Ptr& c : checkable->GetComments()) {
			if (c->GetEntryType() == CommentAcknowledgement) {
				if (c->GetEntryTime() > entry) {
					entry = c->GetEntryTime();
					AckComment = c;
				}
			}
		}
		if (AckComment != nullptr) {
			attrs->Set("acknowledgement_comment_id", GetObjectIdentifier(AckComment));
		}
	}

	{
		auto lastComment (checkable->GetLastComment());

		if (lastComment) {
			attrs->Set("last_comment_id", GetObjectIdentifier(lastComment));
		}
	}

	attrs->Set("in_downtime", checkable->IsInDowntime());

	if (checkable->GetCheckTimeout().IsEmpty())
		attrs->Set("check_timeout", TimestampToMilliseconds(checkable->GetCheckCommand()->GetTimeout()));
	else
		attrs->Set("check_timeout", TimestampToMilliseconds(checkable->GetCheckTimeout()));

	long long lastCheck = TimestampToMilliseconds(checkable->GetLastCheck());
	if (lastCheck > 0)
		attrs->Set("last_update", lastCheck);

	attrs->Set("last_state_change", TimestampToMilliseconds(checkable->GetLastStateChange()));
	attrs->Set("next_check", TimestampToMilliseconds(checkable->GetNextCheck()));
	attrs->Set("next_update", TimestampToMilliseconds(checkable->GetNextUpdate()));

	return attrs;
}

std::vector<String>
IcingaDB::UpdateObjectAttrs(const ConfigObject::Ptr& object, int fieldType,
							   const String& typeNameOverride)
{
	Type::Ptr type = object->GetReflectionType();
	Dictionary::Ptr attrs(new Dictionary);

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if ((field.Attributes & fieldType) == 0)
			continue;

		Value val = object->GetField(fid);

		/* hide attributes which shouldn't be user-visible */
		if (field.Attributes & FANoUserView)
			continue;

		/* hide internal navigation fields */
		if (field.Attributes & FANavigation && !(field.Attributes & (FAConfig | FAState)))
			continue;

		attrs->Set(field.Name, Serialize(val));
	}

	/* Downtimes require in_effect, which is not an attribute */
	Downtime::Ptr downtime = dynamic_pointer_cast<Downtime>(object);
	if (downtime) {
		attrs->Set("in_effect", Serialize(downtime->IsInEffect()));
		attrs->Set("trigger_time", Serialize(TimestampToMilliseconds(downtime->GetTriggerTime())));
	}


	/* Use the name checksum as unique key. */
	String typeName = type->GetName().ToLower();
	if (!typeNameOverride.IsEmpty())
		typeName = typeNameOverride.ToLower();

	return {GetObjectIdentifier(object), JsonEncode(attrs)};
	//m_Rcon->FireAndForgetQuery({"HSET", keyPrefix + typeName, GetObjectIdentifier(object), JsonEncode(attrs)});
}

void IcingaDB::StateChangeHandler(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type)
{
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendStateChange(object, cr, type);
	}
}

void IcingaDB::ReachabilityChangeHandler(const std::set<Checkable::Ptr>& children)
{
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		std::set<DependencyGroup*> seenGroups;
		for (auto& checkable : children) {
			rw->UpdateState(checkable, StateUpdate::Full);
			rw->UpdateDependenciesState(checkable, nullptr, &seenGroups);
		}
	}
}

void IcingaDB::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	if (m_IndexedTypes.find(type.get()) == m_IndexedTypes.end()) {
		return;
	}

	if (object->IsActive()) {
		// Create or update the object config
		for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
			if (rw)
				rw->SendConfigUpdate(object, true);
		}
	} else if (!object->IsActive() &&
			   object->GetExtension("ConfigObjectDeleted")) { // same as in apilistener-configsync.cpp
		// Delete object config
		for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
			if (rw)
				rw->SendConfigDelete(object);
		}
	}
}

void IcingaDB::DowntimeStartedHandler(const Downtime::Ptr& downtime)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendStartedDowntime(downtime);
	}
}

void IcingaDB::DowntimeRemovedHandler(const Downtime::Ptr& downtime)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendRemovedDowntime(downtime);
	}
}

void IcingaDB::NotificationSentToAllUsersHandler(
	const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
	NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text
)
{
	auto rws (ConfigType::GetObjectsByType<IcingaDB>());
	auto sendTime (notification->GetLastNotification());

	if (!rws.empty()) {
		for (auto& rw : rws) {
			rw->SendSentNotification(notification, checkable, users, type, cr, author, text, sendTime);
		}
	}
}

void IcingaDB::CommentAddedHandler(const Comment::Ptr& comment)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendAddedComment(comment);
	}
}

void IcingaDB::CommentRemovedHandler(const Comment::Ptr& comment)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendRemovedComment(comment);
	}
}

void IcingaDB::FlappingChangeHandler(const Checkable::Ptr& checkable, double changeTime)
{
	auto flappingLastChange (checkable->GetFlappingLastChange());

	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendFlappingChange(checkable, changeTime, flappingLastChange);
	}
}

void IcingaDB::NewCheckResultHandler(const Checkable::Ptr& checkable)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->UpdateState(checkable, StateUpdate::Volatile);
		rw->SendNextUpdate(checkable);
	}
}

void IcingaDB::NextCheckUpdatedHandler(const Checkable::Ptr& checkable)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->UpdateState(checkable, StateUpdate::Volatile);
		rw->SendNextUpdate(checkable);
	}
}

void IcingaDB::DependencyGroupChildRegisteredHandler(const Checkable::Ptr& child, const DependencyGroup::Ptr& dependencyGroup)
{
	for (const auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendDependencyGroupChildRegistered(child, dependencyGroup);
	}
}

void IcingaDB::DependencyGroupChildRemovedHandler(const DependencyGroup::Ptr& dependencyGroup, const std::vector<Dependency::Ptr>& dependencies, bool removeGroup)
{
	for (const auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendDependencyGroupChildRemoved(dependencyGroup, dependencies, removeGroup);
	}
}

void IcingaDB::HostProblemChangedHandler(const Service::Ptr& service) {
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		/* Host state changes affect is_handled and severity of services. */
		rw->UpdateState(service, StateUpdate::Full);
	}
}

void IcingaDB::AcknowledgementSetHandler(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool persistent, double changeTime, double expiry)
{
	auto rws (ConfigType::GetObjectsByType<IcingaDB>());

	if (!rws.empty()) {
		for (auto& rw : rws) {
			rw->SendAcknowledgementSet(checkable, author, comment, type, persistent, changeTime, expiry);
		}
	}
}

void IcingaDB::AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const String& removedBy, double changeTime)
{
	auto rws (ConfigType::GetObjectsByType<IcingaDB>());

	if (!rws.empty()) {
		auto rb (Shared<String>::Make(removedBy));
		auto ackLastChange (checkable->GetAcknowledgementLastChange());

		for (auto& rw : rws) {
			rw->SendAcknowledgementCleared(checkable, *rb, changeTime, ackLastChange);
		}
	}
}

void IcingaDB::NotificationUsersChangedHandler(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendNotificationUsersChanged(notification, oldValues, newValues);
	}
}

void IcingaDB::NotificationUserGroupsChangedHandler(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendNotificationUserGroupsChanged(notification, oldValues, newValues);
	}
}

void IcingaDB::TimePeriodRangesChangedHandler(const TimePeriod::Ptr& timeperiod, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendTimePeriodRangesChanged(timeperiod, oldValues, newValues);
	}
}

void IcingaDB::TimePeriodIncludesChangedHandler(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendTimePeriodIncludesChanged(timeperiod, oldValues, newValues);
	}
}

void IcingaDB::TimePeriodExcludesChangedHandler(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendTimePeriodExcludesChanged(timeperiod, oldValues, newValues);
	}
}

void IcingaDB::UserGroupsChangedHandler(const User::Ptr& user, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendGroupsChanged<UserGroup>(user, oldValues, newValues);
	}
}

void IcingaDB::HostGroupsChangedHandler(const Host::Ptr& host, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendGroupsChanged<HostGroup>(host, oldValues, newValues);
	}
}

void IcingaDB::ServiceGroupsChangedHandler(const Service::Ptr& service, const Array::Ptr& oldValues, const Array::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendGroupsChanged<ServiceGroup>(service, oldValues, newValues);
	}
}

void IcingaDB::CommandEnvChangedHandler(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendCommandEnvChanged(command, oldValues, newValues);
	}
}

void IcingaDB::CommandArgumentsChangedHandler(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendCommandArgumentsChanged(command, oldValues, newValues);
	}
}

void IcingaDB::CustomVarsChangedHandler(const ConfigObject::Ptr& object, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues) {
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendCustomVarsChanged(object, oldValues, newValues);
	}
}

void IcingaDB::DeleteRelationship(const String& id, const String& redisKeyWithoutPrefix, bool hasChecksum) {
	Log(LogNotice, "IcingaDB") << "Deleting relationship '" << redisKeyWithoutPrefix << " -> '" << id << "'";

	String redisKey = m_PrefixConfigObject + redisKeyWithoutPrefix;

	std::vector<std::vector<String>> queries;

	if (hasChecksum) {
		queries.push_back({"HDEL", m_PrefixConfigCheckSum + redisKeyWithoutPrefix, id});
	}

	queries.push_back({"HDEL", redisKey, id});
	queries.push_back({
		"XADD", "icinga:runtime", "MAXLEN", "~", "1000000", "*",
		"redis_key", redisKey, "id", id, "runtime_type", "delete"
	});

	m_Rcon->FireAndForgetQueries(queries, Prio::Config);
}

void IcingaDB::DeleteRelationship(const String& id, RedisKey redisKey, bool hasChecksum)
{
	switch (redisKey) {
		case RedisKey::RedundancyGroup:
			DeleteRelationship(id, "redundancygroup", hasChecksum);
			break;
		case RedisKey::DependencyNode:
			DeleteRelationship(id, "dependency:node", hasChecksum);
			break;
		case RedisKey::DependencyEdge:
			DeleteRelationship(id, "dependency:edge", hasChecksum);
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid RedisKey provided"));
	}
}

void IcingaDB::DeleteState(const String& id, RedisKey redisKey, bool hasChecksum) const
{
	String redisKeyWithoutPrefix;
	switch (redisKey) {
		case RedisKey::RedundancyGroupState:
			redisKeyWithoutPrefix = "redundancygroup:state";
			break;
		case RedisKey::DependencyEdgeState:
			redisKeyWithoutPrefix = "dependency:edge:state";
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid state RedisKey provided"));
	}

	Log(LogNotice, "IcingaDB")
		<< "Deleting state " << std::quoted(redisKeyWithoutPrefix.CStr()) << " -> " << std::quoted(id.CStr());

	RedisConnection::Queries hdels;
	if (hasChecksum) {
		hdels.emplace_back(RedisConnection::Query{"HDEL", m_PrefixConfigCheckSum + redisKeyWithoutPrefix, id});
	}
	hdels.emplace_back(RedisConnection::Query{"HDEL", m_PrefixConfigObject + redisKeyWithoutPrefix, id});

	m_Rcon->FireAndForgetQueries(std::move(hdels), Prio::RuntimeStateSync);
	// TODO: This is currently purposefully commented out due to how Icinga DB (Go) handles runtime state
	//       upsert and delete events. See https://github.com/Icinga/icingadb/pull/894 for more details.
	/*m_Rcon->FireAndForgetQueries({{
		"XADD", "icinga:runtime:state", "MAXLEN", "~", "1000000", "*",
		"redis_key", m_PrefixConfigObject + redisKeyWithoutPrefix, "id", id, "runtime_type", "delete"
	}}, Prio::RuntimeStateStream, {0, 1});*/
}

/**
 * Add the provided data to the Redis HMSETs map.
 *
 * Adds the provided data to the Redis HMSETs map for the provided Redis key. The actual Redis key is determined by
 * the provided RedisKey enum. The data will be json encoded before being added to the Redis HMSETs map.
 *
 * @param hMSets The map of RedisConnection::Query you want to add the data to.
 * @param redisKey The key of the Redis object you want to add the data to.
 * @param id Unique Redis identifier for the provided data.
 * @param data The actual data you want to add the Redis HMSETs map.
 */
void IcingaDB::AddDataToHmSets(std::map<String, RedisConnection::Query>& hMSets, RedisKey redisKey, const String& id, const Dictionary::Ptr& data) const
{
	RedisConnection::Query* query;
	switch (redisKey) {
		case RedisKey::RedundancyGroup:
			query = &hMSets[m_PrefixConfigObject + "redundancygroup"];
			break;
		case RedisKey::DependencyNode:
			query = &hMSets[m_PrefixConfigObject + "dependency:node"];
			break;
		case RedisKey::DependencyEdge:
			query = &hMSets[m_PrefixConfigObject + "dependency:edge"];
			break;
		case RedisKey::RedundancyGroupState:
			query = &hMSets[m_PrefixConfigObject + "redundancygroup:state"];
			break;
		case RedisKey::DependencyEdgeState:
			query = &hMSets[m_PrefixConfigObject + "dependency:edge:state"];
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid RedisKey provided"));
	}

	query->emplace_back(id);
	query->emplace_back(JsonEncode(data));
}

/**
 * Execute the provided HMSET values and runtime updates in a single Redis transaction on the provided Redis connection.
 *
 * The HMSETs should just contain the necessary key value pairs to be set in Redis, i.e, without the HMSET command
 * itself. This function will then go through each of the map keys and prepend the HMSET command when transforming the
 * map into valid Redis queries. Likewise, the runtime updates should just contain the key value pairs to be streamed
 * to the icinga:runtime pipeline, and this function will generate a XADD query for each one of the vector elements.
 *
 * @param rcon The Redis connection to execute the transaction on.
 * @param hMSets A map of Redis keys and their respective HMSET values.
 * @param runtimeUpdates A list of dictionaries to be sent to the icinga:runtime stream.
 */
void IcingaDB::ExecuteRedisTransaction(const RedisConnection::Ptr& rcon, std::map<String, RedisConnection::Query>& hMSets,
	const std::vector<Dictionary::Ptr>& runtimeUpdates)
{
	RedisConnection::Queries transaction{{"MULTI"}};
	for (auto& [redisKey, query] : hMSets) {
		if (!query.empty()) {
			query.insert(query.begin(), {"HSET", redisKey});
			transaction.emplace_back(std::move(query));
		}
	}

	for (auto& attrs : runtimeUpdates) {
		RedisConnection::Query xAdd{"XADD", "icinga:runtime", "MAXLEN", "~", "1000000", "*"};

		ObjectLock olock(attrs);
		for (auto& [key, value] : attrs) {
			if (auto streamVal(IcingaToStreamValue(value)); !streamVal.IsEmpty()) {
				xAdd.emplace_back(key);
				xAdd.emplace_back(std::move(streamVal));
			}
		}

		transaction.emplace_back(std::move(xAdd));
	}

	if (transaction.size() > 1) {
		transaction.emplace_back(RedisConnection::Query{"EXEC"});
		if (!runtimeUpdates.empty()) {
			rcon->FireAndForgetQueries(std::move(transaction), Prio::Config, {1});
		} else {
			// This is likely triggered by the initial Redis config dump, so a) we don't need to record the number of
			// affected objects and b) we don't really know how many objects are going to be affected by this tx.
			rcon->FireAndForgetQueries(std::move(transaction), Prio::Config);
		}
	}
}
