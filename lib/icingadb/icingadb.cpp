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

void IcingaDB::Validate(int types, const ValidationUtils& utils)
{
	ObjectImpl<IcingaDB>::Validate(types, utils);

	if (!(types & FAConfig))
		return;

	if (GetEnableTls() && GetCertPath().IsEmpty() != GetKeyPath().IsEmpty()) {
		BOOST_THROW_EXCEPTION(ValidationError(this, std::vector<String>(), "Validation failed: Either both a client certificate (cert_path) and its private key (key_path) or none of them must be given."));
	}
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

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex(),
		GetEnableTls(), GetInsecureNoverify(), GetCertPath(), GetKeyPath(), GetCaPath(), GetCrlPath(),
		GetTlsProtocolmin(), GetCipherList(), GetDebugInfo());
	m_Rcon->SetConnectedCallback([this](boost::asio::yield_context& yc) {
		m_WorkQueue.Enqueue([this]() { OnConnectedHandler(); });
	});
	m_Rcon->Start();

	for (const Type::Ptr& type : GetTypes()) {
		auto ctype (dynamic_cast<ConfigType*>(type.get()));
		if (!ctype)
			continue;

		RedisConnection::Ptr rCon (new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex(),
			GetEnableTls(), GetInsecureNoverify(), GetCertPath(), GetKeyPath(), GetCaPath(), GetCrlPath(),
			GetTlsProtocolmin(), GetCipherList(), GetDebugInfo(), m_Rcon));
		rCon->Start();
		m_Rcons[ctype] = std::move(rCon);
	}

	m_StatsTimer = new Timer();
	m_StatsTimer->SetInterval(1);
	m_StatsTimer->OnTimerExpired.connect([this](const Timer * const&) { PublishStatsTimerHandler(); });
	m_StatsTimer->Start();

	m_WorkQueue.SetName("IcingaDB");

	m_Rcon->SuppressQueryKind(Prio::CheckResult);
	m_Rcon->SuppressQueryKind(Prio::RuntimeStateSync);
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

	std::vector<String> query {"XADD", "icinga:stats", "MAXLEN", "1", "*"};

	{
		ObjectLock statusLock (status);
		for (auto& kv : status) {
			query.emplace_back(kv.first);
			query.emplace_back(JsonEncode(kv.second));
		}
	}

	m_Rcon->FireAndForgetQuery(std::move(query), Prio::Heartbeat);
}

void IcingaDB::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<IcingaDB>::Stop(runtimeRemoved);
}

void IcingaDB::ValidateTlsProtocolmin(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<IcingaDB>::ValidateTlsProtocolmin(lvalue, utils);

	try {
		ResolveTlsProtocolVersion(lvalue());
	} catch (const std::exception& ex) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "tls_protocolmin" }, ex.what()));
	}
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
