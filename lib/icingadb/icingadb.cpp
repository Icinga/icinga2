/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadb.hpp"
#include "icingadb/icingadb-ti.cpp"
#include "icingadb/redisconnection.hpp"
#include "remote/eventqueue.hpp"
#include "base/json.hpp"
#include "icinga/checkable.hpp"
#include "icinga/host.hpp"
#include <boost/algorithm/string.hpp>
#include <memory>
#include <utility>

using namespace icinga;

#define MAX_EVENTS_DEFAULT 5000

using Prio = RedisConnection::QueryPriority;

static const char * const l_LuaPublishStats = R"EOF(

local xa = {'XADD', KEYS[1], '*'}

for i = 1, #ARGV do
	table.insert(xa, ARGV[i])
end

local id = redis.call(unpack(xa))

local xr = redis.call('XRANGE', KEYS[1], '-', '+')
for i = 1, #xr - 1 do
	redis.call('XDEL', KEYS[1], xr[i][1])
end

return id

)EOF";

REGISTER_TYPE(IcingaDB);

IcingaDB::IcingaDB()
	: m_Rcon(nullptr)
{
	m_Rcon = nullptr;

	m_WorkQueue.SetName("IcingaDB");

	m_PrefixConfigObject = "icinga:config:";
	m_PrefixConfigCheckSum = "icinga:checksum:";
	m_PrefixStateObject = "icinga:config:state:";
}

/**
 * Starts the component.
 */
void IcingaDB::Start(bool runtimeCreated)
{
	ObjectImpl<IcingaDB>::Start(runtimeCreated);

	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' started.";

	m_ConfigDumpInProgress = false;
	m_ConfigDumpDone = false;

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex());
	m_Rcon->Start();

	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(15);
	m_ReconnectTimer->OnTimerExpired.connect([this](const Timer * const&) { ReconnectTimerHandler(); });
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	m_StatsTimer = new Timer();
	m_StatsTimer->SetInterval(1);
	m_StatsTimer->OnTimerExpired.connect([this](const Timer * const&) { PublishStatsTimerHandler(); });
	m_StatsTimer->Start();

	m_WorkQueue.SetName("IcingaDB");

	boost::thread thread(&IcingaDB::HandleEvents, this);
	thread.detach();

}

void IcingaDB::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "IcingaDB", "Exception during redis query. Verify that Redis is operational.");

	Log(LogDebug, "IcingaDB")
		<< "Exception during redis operation: " << DiagnosticInformation(exp);
}

void IcingaDB::ReconnectTimerHandler()
{
	m_WorkQueue.Enqueue([this]() { TryToReconnect(); });
}

void IcingaDB::TryToReconnect()
{
	AssertOnWorkQueue();

	if (m_ConfigDumpDone)
		return;
	else
		m_Rcon->Start();

	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	if (m_ConfigDumpInProgress || m_ConfigDumpDone)
		return;

	/* Config dump */
	m_ConfigDumpInProgress = true;
	PublishStats();

	UpdateAllConfigObjects();

	m_ConfigDumpDone = true;

	m_ConfigDumpInProgress = false;
}

void IcingaDB::PublishStatsTimerHandler(void)
{
	m_WorkQueue.Enqueue([this]() { PublishStats(); });
}

void IcingaDB::PublishStats()
{
	AssertOnWorkQueue();

	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Dictionary::Ptr status = GetStats();
	status->Set("config_dump_in_progress", m_ConfigDumpInProgress);
	status->Set("timestamp", TimestampToMilliseconds(Utility::GetTime()));

	std::vector<String> eval ({"EVAL", l_LuaPublishStats, "1", "icinga:stats"});

	{
		ObjectLock statusLock (status);
		for (auto& kv : status) {
			eval.emplace_back(kv.first);
			eval.emplace_back(JsonEncode(kv.second));
		}
	}

	m_Rcon->FireAndForgetQuery(std::move(eval), Prio::Heartbeat);
}

void IcingaDB::HandleEvents()
{
	String queueName = Utility::NewUniqueID();
	EventQueue::Ptr queue = new EventQueue(queueName);
	EventQueue::Register(queueName, queue);

	std::set<String> types;
	types.insert("CheckResult");
	types.insert("AcknowledgementSet");
	types.insert("AcknowledgementCleared");

	queue->SetTypes(types);

	queue->AddClient(this);

	for (;;) {
		Dictionary::Ptr event = queue->WaitForEvent(this);

		if (!event)
			continue;

		m_WorkQueue.Enqueue([this, event]() { SendEvent(event); });
	}

	queue->RemoveClient(this);
	EventQueue::UnregisterIfUnused(queueName, queue);
}

void IcingaDB::SendEvent(const Dictionary::Ptr& event)
{
	AssertOnWorkQueue();

	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String type = event->Get("type");

	if (type == "CheckResult") {
		Checkable::Ptr checkable;

		if (event->Contains("service")) {
			checkable = Service::GetByNamePair(event->Get("host"), event->Get("service"));
		} else {
			checkable = Host::GetByName(event->Get("host"));
		}

		// Update State for icingaweb
		m_WorkQueue.Enqueue([this, checkable]() { UpdateState(checkable); });
	}

	if (type.Contains("Acknowledgement")) {
		Checkable::Ptr checkable;

		if (event->Contains("service")) {
			checkable = Service::GetByNamePair(event->Get("host"), event->Get("service"));
			event->Set("service_id", GetObjectIdentifier(checkable));
		} else {
			checkable = Host::GetByName(event->Get("host"));
			event->Set("host_id", GetObjectIdentifier(checkable));
		}

		if (type == "AcknowledgementSet") {
			Timestamp entry = 0;
			Comment::Ptr AckComment;

			for (const Comment::Ptr& c : checkable->GetComments()) {
				if (c->GetEntryType() == CommentAcknowledgement) {
					if (c->GetEntryTime() > entry) {
						entry = c->GetEntryTime();
						AckComment = c;
						StateChangeHandler(checkable);
					}
				}
			}

			event->Set("comment_id", GetObjectIdentifier(AckComment));
		}
	}
}

void IcingaDB::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<IcingaDB>::Stop(runtimeRemoved);
}

void IcingaDB::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}
