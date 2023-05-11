/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/configuration.hpp"
#include "base/configuration-ti.cpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_TYPE(Configuration);

String Configuration::ApiBindHost;
String Configuration::ApiBindPort{"5665"};
bool Configuration::AttachDebugger{false};
String Configuration::CacheDir;
int Configuration::Concurrency{static_cast<int>(std::thread::hardware_concurrency())};
bool Configuration::ConcurrencyWasModified{false};
String Configuration::ConfigDir;
String Configuration::DataDir;
String Configuration::EventEngine;
String Configuration::IncludeConfDir;
String Configuration::InitRunDir;
String Configuration::LogDir;
String Configuration::ModAttrPath;
String Configuration::ObjectsPath;
String Configuration::PidPath;
String Configuration::PkgDataDir;
String Configuration::PrefixDir;
String Configuration::ProgramData;
int Configuration::RLimitFiles;
int Configuration::RLimitProcesses;
int Configuration::RLimitStack;
String Configuration::RunAsGroup;
String Configuration::RunAsUser;
String Configuration::SpoolDir;
String Configuration::StatePath;
double Configuration::TlsHandshakeTimeout{10};
String Configuration::VarsPath;
String Configuration::ZonesDir;

/* deprecated */
String Configuration::LocalStateDir;
String Configuration::RunDir;
String Configuration::SysconfDir;

/* internal */
bool Configuration::m_ReadOnly{false};

template<typename T>
void HandleUserWrite(const String& name, T *target, const T& value, bool readOnly)
{
	if (readOnly)
		BOOST_THROW_EXCEPTION(ScriptError("Configuration attribute '" + name + "' is read-only."));

	*target = value;
}

String Configuration::GetApiBindHost() const
{
	return Configuration::ApiBindHost;
}

void Configuration::SetApiBindHost(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ApiBindHost", &Configuration::ApiBindHost, val, m_ReadOnly);
}

String Configuration::GetApiBindPort() const
{
	return Configuration::ApiBindPort;
}

void Configuration::SetApiBindPort(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ApiBindPort", &Configuration::ApiBindPort, val, m_ReadOnly);
}

bool Configuration::GetAttachDebugger() const
{
	return Configuration::AttachDebugger;
}

void Configuration::SetAttachDebugger(bool val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("AttachDebugger", &Configuration::AttachDebugger, val, m_ReadOnly);
}

String Configuration::GetCacheDir() const
{
	return Configuration::CacheDir;
}

void Configuration::SetCacheDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("CacheDir", &Configuration::CacheDir, val, m_ReadOnly);
}

int Configuration::GetConcurrency() const
{
	return Configuration::Concurrency;
}

void Configuration::SetConcurrency(int val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("Concurrency", &Configuration::Concurrency, val, m_ReadOnly);
	Configuration::ConcurrencyWasModified = true;
}

String Configuration::GetConfigDir() const
{
	return Configuration::ConfigDir;
}

void Configuration::SetConfigDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ConfigDir", &Configuration::ConfigDir, val, m_ReadOnly);
}

String Configuration::GetDataDir() const
{
	return Configuration::DataDir;
}

void Configuration::SetDataDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("DataDir", &Configuration::DataDir, val, m_ReadOnly);
}

String Configuration::GetEventEngine() const
{
	return Configuration::EventEngine;
}

void Configuration::SetEventEngine(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("EventEngine", &Configuration::EventEngine, val, m_ReadOnly);
}

String Configuration::GetIncludeConfDir() const
{
	return Configuration::IncludeConfDir;
}

void Configuration::SetIncludeConfDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("IncludeConfDir", &Configuration::IncludeConfDir, val, m_ReadOnly);
}

String Configuration::GetInitRunDir() const
{
	return Configuration::InitRunDir;
}

void Configuration::SetInitRunDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("InitRunDir", &Configuration::InitRunDir, val, m_ReadOnly);
}

String Configuration::GetLogDir() const
{
	return Configuration::LogDir;
}

void Configuration::SetLogDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("LogDir", &Configuration::LogDir, val, m_ReadOnly);
}

String Configuration::GetModAttrPath() const
{
	return Configuration::ModAttrPath;
}

void Configuration::SetModAttrPath(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ModAttrPath", &Configuration::ModAttrPath, val, m_ReadOnly);
}

String Configuration::GetObjectsPath() const
{
	return Configuration::ObjectsPath;
}

void Configuration::SetObjectsPath(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ObjectsPath", &Configuration::ObjectsPath, val, m_ReadOnly);
}

String Configuration::GetPidPath() const
{
	return Configuration::PidPath;
}

void Configuration::SetPidPath(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("PidPath", &Configuration::PidPath, val, m_ReadOnly);
}

String Configuration::GetPkgDataDir() const
{
	return Configuration::PkgDataDir;
}

void Configuration::SetPkgDataDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("PkgDataDir", &Configuration::PkgDataDir, val, m_ReadOnly);
}

String Configuration::GetPrefixDir() const
{
	return Configuration::PrefixDir;
}

void Configuration::SetPrefixDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("PrefixDir", &Configuration::PrefixDir, val, m_ReadOnly);
}

String Configuration::GetProgramData() const
{
	return Configuration::ProgramData;
}

void Configuration::SetProgramData(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ProgramData", &Configuration::ProgramData, val, m_ReadOnly);
}

int Configuration::GetRLimitFiles() const
{
	return Configuration::RLimitFiles;
}

void Configuration::SetRLimitFiles(int val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("RLimitFiles", &Configuration::RLimitFiles, val, m_ReadOnly);
}

int Configuration::GetRLimitProcesses() const
{
	return RLimitProcesses;
}

void Configuration::SetRLimitProcesses(int val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("RLimitProcesses", &Configuration::RLimitProcesses, val, m_ReadOnly);
}

int Configuration::GetRLimitStack() const
{
	return Configuration::RLimitStack;
}

void Configuration::SetRLimitStack(int val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("RLimitStack", &Configuration::RLimitStack, val, m_ReadOnly);
}

String Configuration::GetRunAsGroup() const
{
	return Configuration::RunAsGroup;
}

void Configuration::SetRunAsGroup(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("RunAsGroup", &Configuration::RunAsGroup, val, m_ReadOnly);
}

String Configuration::GetRunAsUser() const
{
	return Configuration::RunAsUser;
}

void Configuration::SetRunAsUser(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("RunAsUser", &Configuration::RunAsUser, val, m_ReadOnly);
}

String Configuration::GetSpoolDir() const
{
	return Configuration::SpoolDir;
}

void Configuration::SetSpoolDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("SpoolDir", &Configuration::SpoolDir, val, m_ReadOnly);
}

String Configuration::GetStatePath() const
{
	return Configuration::StatePath;
}

void Configuration::SetStatePath(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("StatePath", &Configuration::StatePath, val, m_ReadOnly);
}

double Configuration::GetTlsHandshakeTimeout() const
{
	return Configuration::TlsHandshakeTimeout;
}

void Configuration::SetTlsHandshakeTimeout(double val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("TlsHandshakeTimeout", &Configuration::TlsHandshakeTimeout, val, m_ReadOnly);
}

String Configuration::GetVarsPath() const
{
	return Configuration::VarsPath;
}

void Configuration::SetVarsPath(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("VarsPath", &Configuration::VarsPath, val, m_ReadOnly);
}

String Configuration::GetZonesDir() const
{
	return Configuration::ZonesDir;
}

void Configuration::SetZonesDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("ZonesDir", &Configuration::ZonesDir, val, m_ReadOnly);
}

String Configuration::GetLocalStateDir() const
{
	return Configuration::LocalStateDir;
}

void Configuration::SetLocalStateDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("LocalStateDir", &Configuration::LocalStateDir, val, m_ReadOnly);
}

String Configuration::GetSysconfDir() const
{
	return Configuration::SysconfDir;
}

void Configuration::SetSysconfDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("SysconfDir", &Configuration::SysconfDir, val, m_ReadOnly);
}

String Configuration::GetRunDir() const
{
	return Configuration::RunDir;
}

void Configuration::SetRunDir(const String& val, bool suppress_events, const Value& cookie)
{
	HandleUserWrite("RunDir", &Configuration::RunDir, val, m_ReadOnly);
}

bool Configuration::GetReadOnly()
{
	return m_ReadOnly;
}

void Configuration::SetReadOnly(bool readOnly)
{
	m_ReadOnly = readOnly;
}
