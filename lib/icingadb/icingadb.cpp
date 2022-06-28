/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadb.hpp"
#include "icingadb/icingadb-ti.cpp"
#include "icingadb/redisconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/eventqueue.hpp"
#include "base/configuration.hpp"
#include "base/json.hpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"
#include "base/tlsutility.hpp"
#include "base/utility.hpp"
#include "icinga/checkable.hpp"
#include "icinga/host.hpp"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <memory>
#include <utility>

using namespace icinga;

#define MAX_EVENTS_DEFAULT 5000

using Prio = RedisConnection::QueryPriority;

String IcingaDB::m_EnvironmentId;
std::mutex IcingaDB::m_EnvironmentIdInitMutex;

REGISTER_TYPE(IcingaDB);

IcingaDB::IcingaDB()
	: m_Rcon(nullptr)
{
	m_RconLocked.store(nullptr);

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

	try {
		InitEnvironmentId();
	} catch (const std::exception& e) {
		BOOST_THROW_EXCEPTION(ValidationError(this, std::vector<String>(),
			String("Validation failed: ") + e.what()));
	}
}

/**
 * Starts the component.
 */
void IcingaDB::Start(bool runtimeCreated)
{
	ObjectImpl<IcingaDB>::Start(runtimeCreated);

	VERIFY(!m_EnvironmentId.IsEmpty());
	PersistEnvironmentId();

	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' started.";

	m_ConfigDumpInProgress = false;
	m_ConfigDumpDone = false;

	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex(),
		GetEnableTls(), GetInsecureNoverify(), GetCertPath(), GetKeyPath(), GetCaPath(), GetCrlPath(),
		GetTlsProtocolmin(), GetCipherList(), GetConnectTimeout(), GetDebugInfo());
	m_RconLocked.store(m_Rcon);

	for (const Type::Ptr& type : GetTypes()) {
		auto ctype (dynamic_cast<ConfigType*>(type.get()));
		if (!ctype)
			continue;

		RedisConnection::Ptr con = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex(),
			GetEnableTls(), GetInsecureNoverify(), GetCertPath(), GetKeyPath(), GetCaPath(), GetCrlPath(),
			GetTlsProtocolmin(), GetCipherList(), GetConnectTimeout(), GetDebugInfo(), m_Rcon);

		con->SetConnectedCallback([this, con](boost::asio::yield_context& yc) {
			con->SetConnectedCallback(nullptr);

			size_t pending = --m_PendingRcons;
			Log(LogDebug, "IcingaDB") << pending << " pending child connections remaining";
			if (pending == 0) {
				m_WorkQueue.Enqueue([this]() { OnConnectedHandler(); });
			}
		});

		m_Rcons[ctype] = std::move(con);
	}

	m_PendingRcons = m_Rcons.size();

	m_Rcon->SetConnectedCallback([this](boost::asio::yield_context& yc) {
		m_Rcon->SetConnectedCallback(nullptr);

		for (auto& kv : m_Rcons) {
			kv.second->Start();
		}
	});
	m_Rcon->Start();

	m_StatsTimer = new Timer();
	m_StatsTimer->SetInterval(1);
	m_StatsTimer->OnTimerExpired.connect([this](const Timer * const&) { PublishStatsTimerHandler(); });
	m_StatsTimer->Start();

	m_WorkQueue.SetName("IcingaDB");

	m_Rcon->SuppressQueryKind(Prio::CheckResult);
	m_Rcon->SuppressQueryKind(Prio::RuntimeStateSync);

	Ptr keepAlive (this);

	m_HistoryThread = std::async(std::launch::async, [this, keepAlive]() { ForwardHistoryEntries(); });
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
	status->Set("icingadb_environment", m_EnvironmentId);

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
		<< "Flushing history data buffer to Redis.";

	if (m_HistoryThread.wait_for(std::chrono::minutes(1)) == std::future_status::timeout) {
		Log(LogCritical, "IcingaDB")
			<< "Flushing takes more than one minute (while we're about to shut down). Giving up and discarding "
			<< m_HistoryBulker.Size() << " queued history queries.";
	}

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

void IcingaDB::ValidateConnectTimeout(const Lazy<double>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<IcingaDB>::ValidateConnectTimeout(lvalue, utils);

	if (lvalue() <= 0) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "connect_timeout" }, "Value must be greater than 0."));
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

String IcingaDB::GetEnvironmentId() const {
	return m_EnvironmentId;
}

bool IcingaDB::DumpedGlobals::IsNew(const String& id)
{
	std::lock_guard<std::mutex> l (m_Mutex);
	return m_Ids.emplace(id).second;
}

/**
 * Initializes the m_EnvironmentId attribute or throws an exception on failure to do so. Can be called concurrently.
 */
void IcingaDB::InitEnvironmentId()
{
	// Initialize m_EnvironmentId once across all IcingaDB objects. In theory, this could be done using
	// std::call_once, however, due to a bug in libstdc++ (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66146),
	// this can result in a deadlock when an exception is thrown (which is explicitly allowed by the standard).
	std::unique_lock<std::mutex> lock (m_EnvironmentIdInitMutex);

	if (m_EnvironmentId.IsEmpty()) {
		String path = Configuration::DataDir + "/icingadb.env";
		String envId;

		if (Utility::PathExists(path)) {
			envId = Utility::LoadJsonFile(path);

			if (envId.GetLength() != 2*SHA_DIGEST_LENGTH) {
				throw std::runtime_error("environment ID stored at " + path + " is corrupt: wrong length.");
			}

			for (unsigned char c : envId) {
				if (!std::isxdigit(c)) {
					throw std::runtime_error("environment ID stored at " + path + " is corrupt: invalid hex string.");
				}
			}
		} else {
			String caPath = ApiListener::GetDefaultCaPath();

			if (!Utility::PathExists(caPath)) {
				throw std::runtime_error("Cannot find the CA certificate at '" + caPath + "'. "
					"Please ensure the ApiListener is enabled first using 'icinga2 api setup'.");
			}

			std::shared_ptr<X509> cert = GetX509Certificate(caPath);

			unsigned int n;
			unsigned char digest[EVP_MAX_MD_SIZE];
			if (X509_pubkey_digest(cert.get(), EVP_sha1(), digest, &n) != 1) {
				BOOST_THROW_EXCEPTION(openssl_error()
											  << boost::errinfo_api_function("X509_pubkey_digest")
											  << errinfo_openssl_error(ERR_peek_error()));
			}

			envId = BinaryToHex(digest, n);
		}

		m_EnvironmentId = envId.ToLower();
	}
}

/**
 * Ensures that the environment ID is persisted on disk or throws an exception on failure to do so.
 * Can be called concurrently.
 */
void IcingaDB::PersistEnvironmentId()
{
	String path = Configuration::DataDir + "/icingadb.env";

	std::unique_lock<std::mutex> lock (m_EnvironmentIdInitMutex);

	if (!Utility::PathExists(path)) {
		Utility::SaveJsonFile(path, 0600, m_EnvironmentId);
	}
}
