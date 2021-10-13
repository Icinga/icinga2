/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadb.hpp"
#include "icingadb/icingadb-ti.cpp"
#include "icingadb/redisconnection.hpp"
#include "remote/apilistener.hpp"
#include "remote/eventqueue.hpp"
#include "base/configuration.hpp"
#include "base/json.hpp"
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
std::once_flag IcingaDB::m_EnvironmentIdOnce;

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

	std::call_once(m_EnvironmentIdOnce, []() {
		String path = Configuration::DataDir + "/icingadb.env";

		if (Utility::PathExists(path)) {
			m_EnvironmentId = Utility::LoadJsonFile(path);

			if (m_EnvironmentId.GetLength() != 2*SHA_DIGEST_LENGTH) {
				throw std::runtime_error("Wrong length of stored Icinga DB environment");
			}

			for (unsigned char c : m_EnvironmentId) {
				if (!std::isxdigit(c)) {
					throw std::runtime_error("Stored Icinga DB environment is not a hex string");
				}
			}
		} else {
			std::shared_ptr<X509> cert = GetX509Certificate(ApiListener::GetDefaultCaPath());

			unsigned int n;
			unsigned char digest[EVP_MAX_MD_SIZE];
			if (X509_pubkey_digest(cert.get(), EVP_sha1(), digest, &n) != 1) {
				BOOST_THROW_EXCEPTION(openssl_error()
					<< boost::errinfo_api_function("X509_pubkey_digest")
					<< errinfo_openssl_error(ERR_peek_error()));
			}

			m_EnvironmentId = BinaryToHex(digest, n);

			Utility::SaveJsonFile(path, 0600, m_EnvironmentId);
		}

		m_EnvironmentId = m_EnvironmentId.ToLower();
	});

	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' started.";

	m_ConfigDumpInProgress = false;
	m_ConfigDumpDone = false;

	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex(),
		GetEnableTls(), GetInsecureNoverify(), GetCertPath(), GetKeyPath(), GetCaPath(), GetCrlPath(),
		GetTlsProtocolmin(), GetCipherList(), GetConnectTimeout(), GetDebugInfo());

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

bool IcingaDB::DumpedGlobals::IsNew(const String& id)
{
	std::lock_guard<std::mutex> l (m_Mutex);
	return m_Ids.emplace(id).second;
}
