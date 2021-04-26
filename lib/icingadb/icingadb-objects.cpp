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
#include "icinga/command.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/customvarobject.hpp"
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
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

using namespace icinga;

using Prio = RedisConnection::QueryPriority;

static const char * const l_LuaResetDump = R"EOF(

local id = redis.call('XADD', KEYS[1], '*', 'type', '*', 'state', 'wip')

local xr = redis.call('XRANGE', KEYS[1], '-', '+')
for i = 1, #xr - 1 do
	redis.call('XDEL', KEYS[1], xr[i][1])
end

return id

)EOF";

INITIALIZE_ONCE(&IcingaDB::ConfigStaticInitialize);

void IcingaDB::ConfigStaticInitialize()
{
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

	Checkable::OnAcknowledgementSet.connect([](const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool, bool persistent, double changeTime, double expiry, const MessageOrigin::Ptr&) {
		IcingaDB::StateChangeHandler(checkable);
	});
	/* triggered when acknowledged host/service goes back to ok and when the acknowledgement gets deleted */
	Checkable::OnAcknowledgementCleared.connect([](const Checkable::Ptr& checkable, const String&, double, const MessageOrigin::Ptr&) {
		IcingaDB::StateChangeHandler(checkable);
	});

	/* triggered on create, update and delete objects */
	ConfigObject::OnActiveChanged.connect([](const ConfigObject::Ptr& object, const Value&) {
		IcingaDB::VersionChangedHandler(object);
	});
	ConfigObject::OnVersionChanged.connect([](const ConfigObject::Ptr& object, const Value&) {
		IcingaDB::VersionChangedHandler(object);
	});

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

	Checkable::OnNextCheckChanged.connect([](const Checkable::Ptr& checkable, const Value&) {
		IcingaDB::NextCheckChangedHandler(checkable);
	});

	Service::OnHostProblemChanged.connect([](const Service::Ptr& service, const CheckResult::Ptr&, const MessageOrigin::Ptr&) {
		IcingaDB::StateChangeHandler(service);
	});
}

void IcingaDB::UpdateAllConfigObjects()
{
	Log(LogInformation, "IcingaDB") << "Starting initial config/status dump";
	double startTime = Utility::GetTime();

	// Use a Workqueue to pack objects in parallel
	WorkQueue upq(25000, Configuration::Concurrency);
	upq.SetName("IcingaDB:ConfigDump");

	typedef std::pair<ConfigType *, String> TypePair;
	std::vector<TypePair> types;

	for (const Type::Ptr& type : {
		CheckCommand::TypeInstance,
		Comment::TypeInstance,
		Downtime::TypeInstance,
		Endpoint::TypeInstance,
		EventCommand::TypeInstance,
		Host::TypeInstance,
		HostGroup::TypeInstance,
		Notification::TypeInstance,
		NotificationCommand::TypeInstance,
		Service::TypeInstance,
		ServiceGroup::TypeInstance,
		TimePeriod::TypeInstance,
		User::TypeInstance,
		UserGroup::TypeInstance,
		Zone::TypeInstance
	}) {
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());
		if (!ctype)
			continue;

		String lcType(type->GetName().ToLower());
		types.emplace_back(ctype, lcType);
	}

	m_Rcon->SuppressQueryKind(Prio::CheckResult);
	m_Rcon->SuppressQueryKind(Prio::State);

	Defer unSuppress ([this]() {
		m_Rcon->UnsuppressQueryKind(Prio::State);
		m_Rcon->UnsuppressQueryKind(Prio::CheckResult);
	});

	m_Rcon->FireAndForgetQuery({"EVAL", l_LuaResetDump, "1", "icinga:dump"}, Prio::Config);

	const std::vector<String> globalKeys = {
			m_PrefixConfigObject + "customvar",
			m_PrefixConfigObject + "action_url",
			m_PrefixConfigObject + "notes_url",
			m_PrefixConfigObject + "icon_image",
	};
	DeleteKeys(globalKeys, Prio::Config);
	DeleteKeys({"icinga:nextupdate:host", "icinga:nextupdate:service"}, Prio::CheckResult);

	Defer resetDumpedGlobals ([this]() {
		m_DumpedGlobals.CustomVar.Reset();
		m_DumpedGlobals.ActionUrl.Reset();
		m_DumpedGlobals.NotesUrl.Reset();
		m_DumpedGlobals.IconImage.Reset();
	});

	upq.ParallelFor(types, [this](const TypePair& type) {
		String lcType = type.second;

		std::vector<String> keys = GetTypeOverwriteKeys(lcType);
		DeleteKeys(keys, Prio::Config);

		WorkQueue upqObjectType(25000, Configuration::Concurrency);
		upqObjectType.SetName("IcingaDB:ConfigDump:" + lcType);

		std::map<String, String> redisCheckSums;
		String configCheckSum = m_PrefixConfigCheckSum + lcType;

		upqObjectType.Enqueue([this, &configCheckSum, &redisCheckSums]() {
			String cursor = "0";

			do {
				Array::Ptr res = m_Rcon->GetResultOfQuery({
					"HSCAN", configCheckSum, cursor, "COUNT", "1000"
				}, Prio::Config);

				Array::Ptr kvs = res->Get(1);
				Value* key = nullptr;
				ObjectLock oLock (kvs);

				for (auto& kv : kvs) {
					if (key) {
						redisCheckSums.emplace(std::move(*key), std::move(kv));
						key = nullptr;
					} else {
						key = &kv;
					}
				}

				cursor = res->Get(0);
			} while (cursor != "0");
		});

		auto objectChunks (ChunkObjects(type.first->GetObjects(), 500));
		String configObject = m_PrefixConfigObject + lcType;

		// Skimmed away attributes and checksums HMSETs' keys and values by Redis key.
		std::map<String, std::vector<std::vector<String>>> ourContentRaw {{configCheckSum, {}}, {configObject, {}}};
		std::mutex ourContentMutex;

		upqObjectType.ParallelFor(objectChunks, [&](decltype(objectChunks)::const_reference chunk) {
			std::map<String, std::vector<String>> hMSets, publishes;
			std::vector<String> states 							= {"HMSET", m_PrefixStateObject + lcType};
			std::vector<std::vector<String> > transaction 		= {{"MULTI"}};
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

				CreateConfigUpdate(object, lcType, hMSets, publishes, false);

				// Write out inital state for checkables
				if (dumpState) {
					states.emplace_back(GetObjectIdentifier(object));
					states.emplace_back(JsonEncode(SerializeState(dynamic_pointer_cast<Checkable>(object))));
				}

				bulkCounter++;
				if (!(bulkCounter % 100)) {
					skimObjects();

					for (auto& kv : hMSets) {
						if (!kv.second.empty()) {
							kv.second.insert(kv.second.begin(), {"HMSET", kv.first});
							transaction.emplace_back(std::move(kv.second));
						}
					}

					if (states.size() > 2) {
						transaction.emplace_back(std::move(states));
						states = {"HMSET", m_PrefixStateObject + lcType};
					}

					for (auto& kv : publishes) {
						for (auto& message : kv.second) {
							std::vector<String> publish;

							publish.reserve(3);
							publish.emplace_back("PUBLISH");
							publish.emplace_back(kv.first);
							publish.emplace_back(std::move(message));

							transaction.emplace_back(std::move(publish));
						}
					}

					hMSets = decltype(hMSets)();
					publishes = decltype(publishes)();

					if (transaction.size() > 1) {
						transaction.push_back({"EXEC"});
						m_Rcon->FireAndForgetQueries(std::move(transaction), Prio::Config);
						transaction = {{"MULTI"}};
					}
				}

				auto checkable (dynamic_pointer_cast<Checkable>(object));

				if (checkable) {
					auto zAdds (dynamic_pointer_cast<Service>(checkable) ? &serviceZAdds : &hostZAdds);

					zAdds->emplace_back(Convert::ToString(checkable->GetNextUpdate()));
					zAdds->emplace_back(GetObjectIdentifier(checkable));

					if (zAdds->size() >= 102u) {
						std::vector<String> header (zAdds->begin(), zAdds->begin() + 2u);

						m_Rcon->FireAndForgetQuery(std::move(*zAdds), Prio::CheckResult);

						*zAdds = std::move(header);
					}
				}
			}

			skimObjects();

			for (auto& kv : hMSets) {
				if (!kv.second.empty()) {
					kv.second.insert(kv.second.begin(), {"HMSET", kv.first});
					transaction.emplace_back(std::move(kv.second));
				}
			}

			if (states.size() > 2)
				transaction.emplace_back(std::move(states));

			for (auto& kv : publishes) {
				for (auto& message : kv.second) {
					std::vector<String> publish;

					publish.reserve(3);
					publish.emplace_back("PUBLISH");
					publish.emplace_back(kv.first);
					publish.emplace_back(std::move(message));

					transaction.emplace_back(std::move(publish));
				}
			}

			if (transaction.size() > 1) {
				transaction.push_back({"EXEC"});
				m_Rcon->FireAndForgetQueries(std::move(transaction), Prio::Config);
			}

			for (auto zAdds : {&hostZAdds, &serviceZAdds}) {
				if (zAdds->size() > 2u) {
					m_Rcon->FireAndForgetQuery(std::move(*zAdds), Prio::CheckResult);
				}
			}

			Log(LogNotice, "IcingaDB")
					<< "Dumped " << bulkCounter << " objects of type " << type.second;
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
			setChecksum.insert(setChecksum.begin(), {"HMSET", configCheckSum});
			setObject.insert(setObject.begin(), {"HMSET", configObject});

			std::vector<std::vector<String>> transaction;

			transaction.emplace_back(std::vector<String>{"MULTI"});
			transaction.emplace_back(std::move(setChecksum));
			transaction.emplace_back(std::move(setObject));
			transaction.emplace_back(std::vector<String>{"EXEC"});

			setChecksum.clear();
			setObject.clear();

			m_Rcon->FireAndForgetQueries(std::move(transaction), Prio::Config);
		});

		auto flushDels ([&]() {
			delChecksum.insert(delChecksum.begin(), {"HDEL", configCheckSum});
			delObject.insert(delObject.begin(), {"HDEL", configObject});

			std::vector<std::vector<String>> transaction;

			transaction.emplace_back(std::vector<String>{"MULTI"});
			transaction.emplace_back(std::move(delChecksum));
			transaction.emplace_back(std::move(delObject));
			transaction.emplace_back(std::vector<String>{"EXEC"});

			delChecksum.clear();
			delObject.clear();

			m_Rcon->FireAndForgetQueries(std::move(transaction), Prio::Config);
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

		m_Rcon->FireAndForgetQuery({"XADD", "icinga:dump", "*", "type", lcType, "state", "done"}, Prio::Config);
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

	m_Rcon->FireAndForgetQuery({"XADD", "icinga:dump", "*", "type", "*", "state", "done"}, Prio::Config);

	// enqueue a callback that will notify us once all previous queries were executed and wait for this event
	std::promise<void> p;
	m_Rcon->EnqueueCallback([&p](boost::asio::yield_context& yc) { p.set_value(); }, Prio::Config);
	p.get_future().wait();

	Log(LogInformation, "IcingaDB")
			<< "Initial config/status dump finished in " << Utility::GetTime() - startTime << " seconds.";
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

	return std::move(chunks);
}

void IcingaDB::DeleteKeys(const std::vector<String>& keys, RedisConnection::QueryPriority priority) {
	std::vector<String> query = {"DEL"};
	for (auto& key : keys) {
		query.emplace_back(key);
	}

	m_Rcon->FireAndForgetQuery(std::move(query), priority);
}

std::vector<String> IcingaDB::GetTypeOverwriteKeys(const String& type)
{
	std::vector<String> keys = {
			m_PrefixConfigObject + type + ":customvar",
	};

	if (type == "host" || type == "service" || type == "user") {
		keys.emplace_back(m_PrefixConfigObject + type + ":groupmember");
		keys.emplace_back(m_PrefixStateObject + type);
	} else if (type == "timeperiod") {
		keys.emplace_back(m_PrefixConfigObject + type + ":override:include");
		keys.emplace_back(m_PrefixConfigObject + type + ":override:exclude");
		keys.emplace_back(m_PrefixConfigObject + type + ":range");
	} else if (type == "zone") {
		keys.emplace_back(m_PrefixConfigObject + type + ":parent");
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

	return std::move(keys);
}

template<typename ConfigType>
static ConfigObject::Ptr GetObjectByName(const String& name)
{
	return ConfigObject::GetObject<ConfigType>(name);
}

void IcingaDB::InsertObjectDependencies(const ConfigObject::Ptr& object, const String typeName, std::map<String, std::vector<String>>& hMSets,
		std::map<String, std::vector<String>>& publishes, bool runtimeUpdate)
{
	String objectKey = GetObjectIdentifier(object);
	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);
	auto env (GetEnvironment());

	if (customVarObject) {
		auto vars(SerializeVars(customVarObject));
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
				}

				if (runtimeUpdate) {
					publishes["icinga:config:update:customvar"].emplace_back(kv.first);
				}

				String id = HashValue(new Array(Prepend(env, Prepend(kv.first, GetObjectIdentifiersWithoutEnv(object)))));
				typeCvs.emplace_back(id);
				typeCvs.emplace_back(JsonEncode(new Dictionary({{"object_id", objectKey}, {"environment_id", m_EnvironmentId}, {"customvar_id", kv.first}})));

				if (runtimeUpdate) {
					publishes["icinga:config:update:" + typeName + ":customvar"].emplace_back(id);
				}
			}
		}
	}

	Type::Ptr type = object->GetReflectionType();
	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

		String actionUrl = checkable->GetActionUrl();
		String notesUrl = checkable->GetNotesUrl();
		String iconImage = checkable->GetIconImage();
		if (!actionUrl.IsEmpty()) {
			auto& actionUrls (hMSets[m_PrefixConfigObject + "action_url"]);

			auto id (HashValue(new Array({env, actionUrl})));

			if (runtimeUpdate || m_DumpedGlobals.ActionUrl.IsNew(id)) {
				actionUrls.emplace_back(std::move(id));
				actionUrls.emplace_back(JsonEncode(new Dictionary({{"environment_id", m_EnvironmentId}, {"action_url", actionUrl}})));
			}

			if (runtimeUpdate) {
				publishes["icinga:config:update:action_url"].emplace_back(actionUrls.at(actionUrls.size() - 2u));
			}
		}
		if (!notesUrl.IsEmpty()) {
			auto& notesUrls (hMSets[m_PrefixConfigObject + "notes_url"]);

			auto id (HashValue(new Array({env, notesUrl})));

			if (runtimeUpdate || m_DumpedGlobals.NotesUrl.IsNew(id)) {
				notesUrls.emplace_back(std::move(id));
				notesUrls.emplace_back(JsonEncode(new Dictionary({{"environment_id", m_EnvironmentId}, {"notes_url", notesUrl}})));
			}

			if (runtimeUpdate) {
				publishes["icinga:config:update:notes_url"].emplace_back(notesUrls.at(notesUrls.size() - 2u));
			}
		}
		if (!iconImage.IsEmpty()) {
			auto& iconImages (hMSets[m_PrefixConfigObject + "icon_image"]);

			auto id (HashValue(new Array({env, iconImage})));

			if (runtimeUpdate || m_DumpedGlobals.IconImage.IsNew(id)) {
				iconImages.emplace_back(std::move(id));
				iconImages.emplace_back(JsonEncode(new Dictionary({{"environment_id", m_EnvironmentId}, {"icon_image", iconImage}})));
			}

			if (runtimeUpdate) {
				publishes["icinga:config:update:icon_image"].emplace_back(iconImages.at(iconImages.size() - 2u));
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

			auto& members (hMSets[m_PrefixConfigObject + typeName + ":groupmember"]);

			for (auto& group : groups) {
				auto groupObj ((*getGroup)(group));
				String groupId = GetObjectIdentifier(groupObj);
				String id = HashValue(new Array(Prepend(env, Prepend(GetObjectIdentifiersWithoutEnv(groupObj), GetObjectIdentifiersWithoutEnv(object)))));
				members.emplace_back(id);
				members.emplace_back(JsonEncode(new Dictionary({{"object_id", objectKey}, {"environment_id", m_EnvironmentId}, {"group_id", groupId}})));

				if (runtimeUpdate) {
					publishes["icinga:config:update:" + typeName + ":groupmember"].emplace_back(id);
				}

				groupIds->Add(groupId);
			}
		}

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
				String rangeId = HashValue(new Array({env, kv.first, kv.second}));
				rangeIds->Add(rangeId);

				String id = HashValue(new Array(Prepend(env, Prepend(kv.first, Prepend(kv.second, GetObjectIdentifiersWithoutEnv(object))))));
				typeRanges.emplace_back(id);
				typeRanges.emplace_back(JsonEncode(new Dictionary({{"environment_id", m_EnvironmentId}, {"timeperiod_id", objectKey}, {"range_key", kv.first}, {"range_value", kv.second}})));

				if (runtimeUpdate) {
					publishes["icinga:config:update:" + typeName + ":range"].emplace_back(id);
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

			String id = HashValue(new Array(Prepend(env, Prepend(GetObjectIdentifiersWithoutEnv(includeTp), GetObjectIdentifiersWithoutEnv(object)))));
			includs.emplace_back(id);
			includs.emplace_back(JsonEncode(new Dictionary({{"environment_id", m_EnvironmentId}, {"timeperiod_id", objectKey}, {"include_id", includeId}})));

			if (runtimeUpdate) {
				publishes["icinga:config:update:" + typeName + ":override:include"].emplace_back(id);
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

			String id = HashValue(new Array(Prepend(env, Prepend(GetObjectIdentifiersWithoutEnv(excludeTp), GetObjectIdentifiersWithoutEnv(object)))));
			excluds.emplace_back(id);
			excluds.emplace_back(JsonEncode(new Dictionary({{"environment_id", m_EnvironmentId}, {"timeperiod_id", objectKey}, {"exclude_id", excludeId}})));

			if (runtimeUpdate) {
				publishes["icinga:config:update:" + typeName + ":override:exclude"].emplace_back(id);
			}
		}

		return;
	}

	if (type == Zone::TypeInstance) {
		Zone::Ptr zone = static_pointer_cast<Zone>(object);

		Array::Ptr parents(new Array);
		auto parentsRaw (zone->GetAllParentsRaw());

		parents->Reserve(parentsRaw.size());

		auto& parnts (hMSets[m_PrefixConfigObject + typeName + ":parent"]);

		for (auto& parent : parentsRaw) {
			String id = HashValue(new Array(Prepend(env, Prepend(GetObjectIdentifiersWithoutEnv(parent), GetObjectIdentifiersWithoutEnv(object)))));
			parnts.emplace_back(id);
			parnts.emplace_back(JsonEncode(new Dictionary({{"zone_id", objectKey}, {"environment_id", m_EnvironmentId}, {"parent_id", GetObjectIdentifier(parent)}})));

			if (runtimeUpdate) {
				publishes["icinga:config:update:" + typeName + ":parent"].emplace_back(id);
			}

			parents->Add(GetObjectIdentifier(parent));
		}

		return;
	}

	if (type == User::TypeInstance) {
		User::Ptr user = static_pointer_cast<User>(object);

		Array::Ptr groups;
		ConfigObject::Ptr (*getGroup)(const String& name);

		groups = user->GetGroups();
		getGroup = &::GetObjectByName<UserGroup>;

		if (groups) {
			ObjectLock groupsLock(groups);
			Array::Ptr groupIds(new Array);

			groupIds->Reserve(groups->GetLength());

			auto& members (hMSets[m_PrefixConfigObject + typeName + ":groupmember"]);

			for (auto& group : groups) {
				auto groupObj ((*getGroup)(group));
				String groupId = GetObjectIdentifier(groupObj);
				String id = HashValue(new Array(Prepend(env, Prepend(GetObjectIdentifiersWithoutEnv(groupObj), GetObjectIdentifiersWithoutEnv(object)))));
				members.emplace_back(id);
				members.emplace_back(JsonEncode(new Dictionary({{"user_id", objectKey}, {"environment_id", m_EnvironmentId}, {"group_id", groupId}})));

				if (runtimeUpdate) {
					publishes["icinga:config:update:" + typeName + ":groupmember"].emplace_back(id);
				}

				groupIds->Add(groupId);
			}
		}

		return;
	}

	if (type == Notification::TypeInstance) {
		Notification::Ptr notification = static_pointer_cast<Notification>(object);

		std::set<User::Ptr> users = notification->GetUsers();

		std::set<User::Ptr> allUsers;
		std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

		Array::Ptr userIds = new Array();

		auto usergroups(notification->GetUserGroups());
		Array::Ptr usergroupIds = new Array();

		userIds->Reserve(users.size());

		auto& usrs (hMSets[m_PrefixConfigObject + typeName + ":user"]);

		for (auto& user : users) {
			String userId = GetObjectIdentifier(user);
			String id = HashValue(new Array(Prepend(env, Prepend(GetObjectIdentifiersWithoutEnv(user), GetObjectIdentifiersWithoutEnv(object)))));
			usrs.emplace_back(id);
			usrs.emplace_back(JsonEncode(new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"user_id", userId}})));

			if (runtimeUpdate) {
				publishes["icinga:config:update:" + typeName + ":user"].emplace_back(id);
			}

			userIds->Add(userId);
		}

		usergroupIds->Reserve(usergroups.size());

		auto& groups (hMSets[m_PrefixConfigObject + typeName + ":usergroup"]);
		auto& notificationRecipients (hMSets[m_PrefixConfigObject + typeName + ":recipient"]);

		for (auto& usergroup : usergroups) {
			String usergroupId = GetObjectIdentifier(usergroup);

			auto groupMembers = usergroup->GetMembers();
			std::copy(groupMembers.begin(), groupMembers.end(), std::inserter(allUsers, allUsers.begin()));

			String id = HashValue(new Array(Prepend(env, Prepend("usergroup", Prepend(GetObjectIdentifiersWithoutEnv(usergroup), GetObjectIdentifiersWithoutEnv(object))))));
			groups.emplace_back(id);
			groups.emplace_back(JsonEncode(new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"usergroup_id", usergroupId}})));

			notificationRecipients.emplace_back(id);
			notificationRecipients.emplace_back(JsonEncode(new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"usergroup_id", usergroupId}})));

			if (runtimeUpdate) {
				publishes["icinga:config:update:" + typeName + ":usergroup"].emplace_back(id);
			}

			usergroupIds->Add(usergroupId);
		}

		for (auto& user : allUsers) {
			String userId = GetObjectIdentifier(user);
			String id = HashValue(new Array(Prepend(env, Prepend("user", Prepend(GetObjectIdentifiersWithoutEnv(user), GetObjectIdentifiersWithoutEnv(object))))));
			notificationRecipients.emplace_back(id);
			notificationRecipients.emplace_back(JsonEncode(new Dictionary({{"notification_id", objectKey}, {"environment_id", m_EnvironmentId}, {"user_id", userId}})));

			if (runtimeUpdate) {
				publishes["icinga:config:update:" + typeName + ":recipient"].emplace_back(id);
			}
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

				{
					Value value;

					// JsonEncode() the value if it's set.
					if (values->Get("value", &value)) {
						values->Set("value", JsonEncode(value));
					}
				}

				values->Set("command_id", objectKey);
				values->Set("argument_key", kv.first);
				values->Set("environment_id", m_EnvironmentId);

				String id = HashValue(new Array(Prepend(env, Prepend(kv.first, GetObjectIdentifiersWithoutEnv(object)))));

				typeArgs.emplace_back(id);
				typeArgs.emplace_back(JsonEncode(values));

				if (runtimeUpdate) {
					publishes["icinga:config:update:" + typeName + ":argument"].emplace_back(id);
				}

				argChksms.emplace_back(id);
				argChksms.emplace_back(JsonEncode(new Dictionary({{"checksum", HashValue(kv.second)}})));
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

				values->Set("command_id", objectKey);
				values->Set("envvar_key", kv.first);
				values->Set("environment_id", m_EnvironmentId);

				String id = HashValue(new Array(Prepend(env, Prepend(kv.first, GetObjectIdentifiersWithoutEnv(object)))));

				typeVars.emplace_back(id);
				typeVars.emplace_back(JsonEncode(values));

				if (runtimeUpdate) {
					publishes["icinga:config:update:" + typeName + ":envvar"].emplace_back(id);
				}

				varChksms.emplace_back(id);
				varChksms.emplace_back(JsonEncode(new Dictionary({{"checksum", HashValue(kv.second)}})));
			}
		}

		return;
	}
}

void IcingaDB::UpdateState(const Checkable::Ptr& checkable)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Dictionary::Ptr stateAttrs = SerializeState(checkable);

	m_Rcon->FireAndForgetQuery({"HSET", m_PrefixStateObject + GetLowerCaseTypeNameDB(checkable), GetObjectIdentifier(checkable), JsonEncode(stateAttrs)}, Prio::State);
}

// Used to update a single object, used for runtime updates
void IcingaDB::SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String typeName = GetLowerCaseTypeNameDB(object);

	std::map<String, std::vector<String>> hMSets, publishes;
	std::vector<String> states 							= {"HMSET", m_PrefixStateObject + typeName};

	CreateConfigUpdate(object, typeName, hMSets, publishes, runtimeUpdate);
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (checkable) {
		String objectKey = GetObjectIdentifier(object);
		m_Rcon->FireAndForgetQuery({"HSET", m_PrefixStateObject + typeName, objectKey, JsonEncode(SerializeState(checkable))}, Prio::State);
		publishes["icinga:config:update:state:" + typeName].emplace_back(objectKey);
	}

	std::vector<std::vector<String> > transaction = {{"MULTI"}};

	for (auto& kv : hMSets) {
		if (!kv.second.empty()) {
			kv.second.insert(kv.second.begin(), {"HMSET", kv.first});
			transaction.emplace_back(std::move(kv.second));
		}
	}

	for (auto& kv : publishes) {
		for (auto& message : kv.second) {
			std::vector<String> publish;

			publish.reserve(3);
			publish.emplace_back("PUBLISH");
			publish.emplace_back(kv.first);
			publish.emplace_back(std::move(message));

			transaction.emplace_back(std::move(publish));
		}
	}

	if (transaction.size() > 1) {
		transaction.push_back({"EXEC"});
		m_Rcon->FireAndForgetQueries(std::move(transaction), Prio::Config);
	}

	if (checkable) {
		SendNextUpdate(checkable);
	}
}

// Takes object and collects IcingaDB relevant attributes and computes checksums. Returns whether the object is relevant
// for IcingaDB.
bool IcingaDB::PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes, Dictionary::Ptr& checksums)
{
	attributes->Set("name_checksum", SHA1(object->GetName()));
	attributes->Set("environment_id", m_EnvironmentId);
	attributes->Set("name", object->GetName());

	Zone::Ptr ObjectsZone;
	Type::Ptr type = object->GetReflectionType();

	if (type == Endpoint::TypeInstance) {
		ObjectsZone = static_cast<Endpoint*>(object.get())->GetZone();
	} else {
		ObjectsZone = static_pointer_cast<Zone>(object->GetZone());
	}

	if (ObjectsZone) {
		attributes->Set("zone_id", GetObjectIdentifier(ObjectsZone));
		attributes->Set("zone", ObjectsZone->GetName());
	}

	if (type == Endpoint::TypeInstance) {
		return true;
	}

	if (type == Zone::TypeInstance) {
		Zone::Ptr zone = static_pointer_cast<Zone>(object);

		attributes->Set("is_global", zone->GetGlobal());

		Zone::Ptr parent = zone->GetParent();
		if (parent) {
			attributes->Set("parent_id", GetObjectIdentifier(zone));
		}

		auto parentsRaw (zone->GetAllParentsRaw());
		attributes->Set("depth", parentsRaw.size());

		return true;
	}

	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

		attributes->Set("checkcommand", checkable->GetCheckCommand()->GetName());
		attributes->Set("max_check_attempts", checkable->GetMaxCheckAttempts());
		attributes->Set("check_timeout", checkable->GetCheckTimeout());
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

		attributes->Set("checkcommand_id", GetObjectIdentifier(checkable->GetCheckCommand()));

		Endpoint::Ptr commandEndpoint = checkable->GetCommandEndpoint();
		if (commandEndpoint) {
			attributes->Set("command_endpoint_id", GetObjectIdentifier(commandEndpoint));
			attributes->Set("command_endpoint", commandEndpoint->GetName());
		}

		TimePeriod::Ptr timePeriod = checkable->GetCheckPeriod();
		if (timePeriod) {
			attributes->Set("check_timeperiod_id", GetObjectIdentifier(timePeriod));
			attributes->Set("check_timeperiod", timePeriod->GetName());
		}

		EventCommand::Ptr eventCommand = checkable->GetEventCommand();
		if (eventCommand) {
			attributes->Set("eventcommand_id", GetObjectIdentifier(eventCommand));
			attributes->Set("eventcommand", eventCommand->GetName());
		}

		String actionUrl = checkable->GetActionUrl();
		String notesUrl = checkable->GetNotesUrl();
		String iconImage = checkable->GetIconImage();
		if (!actionUrl.IsEmpty())
			attributes->Set("action_url_id", HashValue(new Array({GetEnvironment(), actionUrl})));
		if (!notesUrl.IsEmpty())
			attributes->Set("notes_url_id", HashValue(new Array({GetEnvironment(), notesUrl})));
		if (!iconImage.IsEmpty())
			attributes->Set("icon_image_id", HashValue(new Array({GetEnvironment(), iconImage})));


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

		attributes->Set("command_id", GetObjectIdentifier(notification->GetCommand()));

		attributes->Set("host_id", GetObjectIdentifier(host));
		if (service)
			attributes->Set("service_id", GetObjectIdentifier(service));

		TimePeriod::Ptr timeperiod = notification->GetPeriod();
		if (timeperiod)
			attributes->Set("timeperiod_id", GetObjectIdentifier(timeperiod));

		if (notification->GetTimes()) {
			attributes->Set("times_begin", notification->GetTimes()->Get("begin"));
			attributes->Set("times_end",notification->GetTimes()->Get("end"));
		}

		attributes->Set("notification_interval", notification->GetInterval());
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
		attributes->Set("is_sticky", comment->GetEntryType() == CommentAcknowledgement && comment->GetCheckable()->GetAcknowledgement() == AcknowledgementSticky);

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
		attributes->Set("flexible_duration", TimestampToMilliseconds(downtime->GetDuration()));
		attributes->Set("is_flexible", !downtime->GetFixed());
		attributes->Set("is_in_effect", downtime->IsInEffect());
		if (downtime->IsInEffect()) {
			attributes->Set("start_time", TimestampToMilliseconds(downtime->GetTriggerTime()));
			attributes->Set("end_time", TimestampToMilliseconds(downtime->GetFixed() ? downtime->GetEndTime() : (downtime->GetTriggerTime() + downtime->GetDuration())));
		}

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
		attributes->Set("timeout", command->GetTimeout());

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
								std::map<String, std::vector<String>>& publishes, bool runtimeUpdate)
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

	InsertObjectDependencies(object, typeName, hMSets, publishes, runtimeUpdate);

	String objectKey = GetObjectIdentifier(object);
	auto& attrs (hMSets[m_PrefixConfigObject + typeName]);
	auto& chksms (hMSets[m_PrefixConfigCheckSum + typeName]);

	attrs.emplace_back(objectKey);
	attrs.emplace_back(JsonEncode(attr));

	chksms.emplace_back(objectKey);
	chksms.emplace_back(JsonEncode(new Dictionary({{"checksum", HashValue(attr)}})));

	/* Send an update event to subscribers. */
	if (runtimeUpdate) {
		publishes["icinga:config:update:" + typeName].emplace_back(objectKey);
	}
}

void IcingaDB::SendConfigDelete(const ConfigObject::Ptr& object)
{
	String typeName = object->GetReflectionType()->GetName().ToLower();
	String objectKey = GetObjectIdentifier(object);

	m_Rcon->FireAndForgetQueries({
								   {"HDEL",    m_PrefixConfigObject + typeName, objectKey},
								   {"DEL",     m_PrefixStateObject + typeName + ":" + objectKey},
								   {"PUBLISH", "icinga:config:delete:" + typeName, objectKey}
						   }, Prio::Config);

	auto checkable (dynamic_pointer_cast<Checkable>(object));

	if (checkable) {
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

void IcingaDB::SendStatusUpdate(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (!checkable)
		return;

	Host::Ptr host;
	Service::Ptr service;

	tie(host, service) = GetHostService(checkable);

	String streamname;
	if (service)
		streamname = "icinga:state:stream:service";
	else
		streamname = "icinga:state:stream:host";

	Dictionary::Ptr objectAttrs = SerializeState(checkable);

	std::vector<String> streamadd({"XADD", streamname, "*"});
	ObjectLock olock(objectAttrs);
	for (const Dictionary::Pair& kv : objectAttrs) {
		streamadd.emplace_back(kv.first);
		streamadd.emplace_back(Utility::ValidateUTF8(kv.second));
	}

	m_Rcon->FireAndForgetQuery(std::move(streamadd), Prio::State);

	int hard_state;
	if (!cr) {
		hard_state = 99;
	} else {
		hard_state = service ? Convert::ToLong(service->GetLastHardState()) : Convert::ToLong(host->GetLastHardState());
	}

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:state", "*",
		"id", Utility::NewUniqueID(),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"state_type", Convert::ToString(type),
		"soft_state", Convert::ToString(cr ? service ? Convert::ToLong(cr->GetState()) : Convert::ToLong(Host::CalculateState(cr->GetState())) : 99),
		"hard_state", Convert::ToString(hard_state),
		"attempt", Convert::ToString(checkable->GetCheckAttempt()),
		"previous_soft_state", Convert::ToString(GetPreviousState(checkable, service, StateTypeSoft)),
		"previous_hard_state", Convert::ToString(GetPreviousState(checkable, service, StateTypeHard)),
		"max_check_attempts", Convert::ToString(checkable->GetMaxCheckAttempts()),
		"event_time", Convert::ToString(TimestampToMilliseconds(cr ? cr->GetExecutionEnd() : Utility::GetTime())),
		"event_id", Utility::NewUniqueID(),
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

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendSentNotification(
	const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
	NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text
)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	auto finalText = text;
	if (finalText == "" && cr) {
		finalText = cr->GetOutput();
	}

	auto usersAmount (users.size());
	auto notificationHistoryId = Utility::NewUniqueID();

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:notification", "*",
		"id", notificationHistoryId,
		"environment_id", m_EnvironmentId,
		"notification_id", GetObjectIdentifier(notification),
		"host_id", GetObjectIdentifier(host),
		"type", Convert::ToString(type),
		"state", Convert::ToString(cr ? service ? Convert::ToLong(cr->GetState()) : Convert::ToLong(Host::CalculateState(cr->GetState())) : 99),
		"previous_hard_state", Convert::ToString(GetPreviousState(checkable, service, StateTypeHard)),
		"author", Utility::ValidateUTF8(author),
		"text", Utility::ValidateUTF8(finalText),
		"users_notified", Convert::ToString(usersAmount),
		"send_time", Convert::ToString(TimestampToMilliseconds(Utility::GetTime())),
		"event_id", Utility::NewUniqueID(),
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

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);

	for (const User::Ptr& user : users) {
		auto userId = GetObjectIdentifier(user);
		std::vector<String> xAddUser ({
			"XADD", "icinga:history:stream:usernotification", "*",
			"id", Utility::NewUniqueID(),
			"environment_id", m_EnvironmentId,
			"notification_history_id", notificationHistoryId,
			"user_id", GetObjectIdentifier(user),
		});

		m_Rcon->FireAndForgetQuery(std::move(xAddUser), Prio::History);
	}
}

void IcingaDB::SendStartedDowntime(const Downtime::Ptr& downtime)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	SendConfigUpdate(downtime, true);

	auto checkable (downtime->GetCheckable());
	auto triggeredBy (Downtime::GetByName(downtime->GetTriggeredBy()));

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:downtime", "*",
		"downtime_id", GetObjectIdentifier(downtime),
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"entry_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEntryTime())),
		"author", Utility::ValidateUTF8(downtime->GetAuthor()),
		"comment", Utility::ValidateUTF8(downtime->GetComment()),
		"is_flexible", Convert::ToString((unsigned short)!downtime->GetFixed()),
		"flexible_duration", Convert::ToString(downtime->GetDuration()),
		"scheduled_start_time", Convert::ToString(TimestampToMilliseconds(downtime->GetStartTime())),
		"scheduled_end_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEndTime())),
		"has_been_cancelled", Convert::ToString((unsigned short)downtime->GetWasCancelled()),
		"trigger_time", Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime())),
		"event_id", Utility::NewUniqueID(),
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
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime() + downtime->GetDuration())));
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendRemovedDowntime(const Downtime::Ptr& downtime)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	auto checkable (downtime->GetCheckable());
	auto triggeredBy (Downtime::GetByName(downtime->GetTriggeredBy()));

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	// Downtime never got triggered (didn't send "downtime_start") so we don't want to send "downtime_end"
	if (downtime->GetTriggerTime() == 0)
		return;

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
		"flexible_duration", Convert::ToString(downtime->GetDuration()),
		"scheduled_start_time", Convert::ToString(TimestampToMilliseconds(downtime->GetStartTime())),
		"scheduled_end_time", Convert::ToString(TimestampToMilliseconds(downtime->GetEndTime())),
		"has_been_cancelled", Convert::ToString((unsigned short)downtime->GetWasCancelled()),
		"trigger_time", Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime())),
		"cancel_time", Convert::ToString(TimestampToMilliseconds(Utility::GetTime())),
		"event_id", Utility::NewUniqueID(),
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
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(downtime->GetTriggerTime() + downtime->GetDuration())));
	}

	auto endpoint (Endpoint::GetLocalEndpoint());

	if (endpoint) {
		xAdd.emplace_back("endpoint_id");
		xAdd.emplace_back(GetObjectIdentifier(endpoint));
	}

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendAddedComment(const Comment::Ptr& comment)
{
	if (!m_Rcon || !m_Rcon->IsConnected() || comment->GetEntryType() != CommentUser)
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
		"is_sticky", Convert::ToString((unsigned short)(comment->GetEntryType() == CommentAcknowledgement && comment->GetCheckable()->GetAcknowledgement() == AcknowledgementSticky)),
		"event_id", Utility::NewUniqueID(),
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

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendRemovedComment(const Comment::Ptr& comment)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
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
		"is_sticky", Convert::ToString((unsigned short)(comment->GetEntryType() == CommentAcknowledgement && comment->GetCheckable()->GetAcknowledgement() == AcknowledgementSticky)),
		"event_id", Utility::NewUniqueID(),
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

	if (comment->GetExpireTime() < Utility::GetTime()) {
		xAdd.emplace_back("remove_time");
		xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(Utility::GetTime())));
		xAdd.emplace_back("has_been_removed");
		xAdd.emplace_back("1");
		xAdd.emplace_back("removed_by");
		xAdd.emplace_back(Utility::ValidateUTF8(comment->GetRemovedBy()));
	} else {
		xAdd.emplace_back("has_been_removed");
		xAdd.emplace_back("0");
	}

	{
		auto expireTime (comment->GetExpireTime());

		if (expireTime > 0) {
			xAdd.emplace_back("expire_time");
			xAdd.emplace_back(Convert::ToString(TimestampToMilliseconds(expireTime)));
		}
	}

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendFlappingChange(const Checkable::Ptr& checkable, double changeTime, double flappingLastChange)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:flapping", "*",
		"environment_id", m_EnvironmentId,
		"host_id", GetObjectIdentifier(host),
		"flapping_threshold_low", Convert::ToString(checkable->GetFlappingThresholdLow()),
		"flapping_threshold_high", Convert::ToString(checkable->GetFlappingThresholdHigh()),
		"event_id", Utility::NewUniqueID()
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
	xAdd.emplace_back("id");
	xAdd.emplace_back(HashValue(new Array({GetEnvironment(), checkable->GetReflectionType()->GetName(), checkable->GetName(), startTime})));

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendNextUpdate(const Checkable::Ptr& checkable)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	m_Rcon->FireAndForgetQuery(
		{
			"ZADD",
			dynamic_pointer_cast<Service>(checkable) ? "icinga:nextupdate:service" : "icinga:nextupdate:host",
			Convert::ToString(checkable->GetNextUpdate()),
			GetObjectIdentifier(checkable)
		},
		Prio::CheckResult
	);
}

void IcingaDB::SendAcknowledgementSet(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool persistent, double changeTime, double expiry)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:acknowledgement", "*",
		"event_id", Utility::NewUniqueID(),
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
	xAdd.emplace_back("id");
	xAdd.emplace_back(HashValue(new Array({GetEnvironment(), checkable->GetReflectionType()->GetName(), checkable->GetName(), setTime})));

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

void IcingaDB::SendAcknowledgementCleared(const Checkable::Ptr& checkable, const String& removedBy, double changeTime, double ackLastChange)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	std::vector<String> xAdd ({
		"XADD", "icinga:history:stream:acknowledgement", "*",
		"event_id", Utility::NewUniqueID(),
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
	xAdd.emplace_back("id");
	xAdd.emplace_back(HashValue(new Array({GetEnvironment(), checkable->GetReflectionType()->GetName(), checkable->GetName(), setTime})));

	if (!removedBy.IsEmpty()) {
		xAdd.emplace_back("cleared_by");
		xAdd.emplace_back(removedBy);
	}

	m_Rcon->FireAndForgetQuery(std::move(xAdd), Prio::History);
}

Dictionary::Ptr IcingaDB::SerializeState(const Checkable::Ptr& checkable)
{
	Dictionary::Ptr attrs = new Dictionary();

	Host::Ptr host;
	Service::Ptr service;

	tie(host, service) = GetHostService(checkable);

	attrs->Set("id", GetObjectIdentifier(checkable));;
	attrs->Set("environment_id", m_EnvironmentId);
	attrs->Set("state_type", checkable->HasBeenChecked() ? checkable->GetStateType() : StateTypeHard);

	// TODO: last_hard/soft_state should be "previous".
	if (service) {
		auto state = service->HasBeenChecked() ? service->GetState() : 99;
		attrs->Set("state", state);
		attrs->Set("hard_state", service->HasBeenChecked() ? service->GetLastHardState() : 99);
		attrs->Set("severity", service->GetSeverity());
	} else {
		auto state = host->HasBeenChecked() ? host->GetState() : 99;
		attrs->Set("state", state);
		attrs->Set("hard_state", host->HasBeenChecked() ? host->GetLastHardState() : 99);
		attrs->Set("severity", host->GetSeverity());
	}

	attrs->Set("previous_hard_state", GetPreviousState(checkable, service, StateTypeHard));
	attrs->Set("check_attempt", checkable->GetCheckAttempt());

	attrs->Set("is_active", checkable->IsActive());

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

		if (!cr->GetCommand().IsEmpty())
			attrs->Set("commandline", FormatCommandLine(cr->GetCommand()));
		attrs->Set("execution_time", TimestampToMilliseconds(fmax(0.0, cr->CalculateExecutionTime())));
		attrs->Set("latency", TimestampToMilliseconds(cr->CalculateLatency()));
		attrs->Set("check_source", cr->GetCheckSource());
	}

	attrs->Set("is_problem", checkable->GetProblem());
	attrs->Set("is_handled", checkable->GetHandled());
	attrs->Set("is_reachable", checkable->IsReachable());
	attrs->Set("is_flapping", checkable->IsFlapping());

	attrs->Set("acknowledgement", checkable->GetAcknowledgement());
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

void IcingaDB::StateChangeHandler(const ConfigObject::Ptr& object)
{
	auto checkable (dynamic_pointer_cast<Checkable>(object));

	if (checkable) {
		IcingaDB::StateChangeHandler(object, checkable->GetLastCheckResult(), checkable->GetStateType());
	}
}

void IcingaDB::StateChangeHandler(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type)
{
	for (const IcingaDB::Ptr& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendStatusUpdate(object, cr, type);
	}
}

void IcingaDB::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

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
	StateChangeHandler(downtime->GetCheckable());

	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->SendStartedDowntime(downtime);
	}
}

void IcingaDB::DowntimeRemovedHandler(const Downtime::Ptr& downtime)
{
	StateChangeHandler(downtime->GetCheckable());

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

	if (!rws.empty()) {
		for (auto& rw : rws) {
			rw->SendSentNotification(notification, checkable, users, type, cr, author, text);
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
		rw->UpdateState(checkable);
		rw->SendNextUpdate(checkable);
	}
}

void IcingaDB::NextCheckChangedHandler(const Checkable::Ptr& checkable)
{
	for (auto& rw : ConfigType::GetObjectsByType<IcingaDB>()) {
		rw->UpdateState(checkable);
		rw->SendNextUpdate(checkable);
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
