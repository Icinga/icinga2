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

String IcingaDB::m_EnvironmentId;
boost::once_flag IcingaDB::m_EnvironmentIdOnce = BOOST_ONCE_INIT;

REGISTER_TYPE(IcingaDB);

IcingaDB::IcingaDB()
	: m_Rcon(nullptr)
{
	m_Rcon = nullptr;

	m_WorkQueue.SetName("IcingaDB");

	m_PrefixConfigObject = "icinga:";
	m_PrefixConfigCheckSum = "icinga:checksum:";
}

/**
 * Starts the component.
 */
void IcingaDB::Start(bool runtimeCreated)
{
	ObjectImpl<IcingaDB>::Start(runtimeCreated);

	boost::call_once([]() {
		m_EnvironmentId = SHA1(GetEnvironment());
	}, m_EnvironmentIdOnce);

	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' started.";

	m_ConfigDumpInProgress = false;
	m_ConfigDumpDone = false;

	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex());
	m_Rcon->SetConnectedCallback([this](boost::asio::yield_context& yc) {
		m_WorkQueue.Enqueue([this]() { OnConnectedHandler(); });
	});
	m_Rcon->Start();

	m_StatsTimer = new Timer();
	m_StatsTimer->SetInterval(1);
	m_StatsTimer->OnTimerExpired.connect([this](const Timer * const&) { PublishStatsTimerHandler(); });
	m_StatsTimer->Start();

	m_WorkQueue.SetName("IcingaDB");

	m_Rcon->SuppressQueryKind(Prio::CheckResult);
	m_Rcon->SuppressQueryKind(Prio::State);
}

void IcingaDB::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "IcingaDB", "Exception during redis query. Verify that Redis is operational.");

	Log(LogDebug, "IcingaDB")
		<< "Exception during redis operation: " << DiagnosticInformation(exp);
}

void IcingaDB::OnConnectedHandler()
{
	AssertOnWorkQueue();

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
	PublishStats();
}

void IcingaDB::PublishStats()
{
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

void IcingaDB::DumpedGlobals::Reset()
{
	std::lock_guard<std::mutex> l (m_Mutex);
	m_Ids.clear();
}

bool IcingaDB::DumpedGlobals::IsNew(const String& id)
{
	std::lock_guard<std::mutex> l (m_Mutex);
	return m_Ids.emplace(id).second;
}
